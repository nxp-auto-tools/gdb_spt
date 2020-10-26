/* Python interface to values.

   Copyright (C) 2008-2020 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include "defs.h"
#include "charset.h"
#include "value.h"
#include "language.h"
#include "target-float.h"
#include "valprint.h"
#include "infcall.h"
#include "expression.h"
#include "cp-abi.h"
#include "python.h"

#include "python-internal.h"

/* Even though Python scalar types directly map to host types, we use
   target types here to remain consistent with the values system in
   GDB (which uses target arithmetic).  */

/* Python's integer type corresponds to C's long type.  */
#define builtin_type_pyint builtin_type (python_gdbarch)->builtin_long

/* Python's float type corresponds to C's double type.  */
#define builtin_type_pyfloat builtin_type (python_gdbarch)->builtin_double

/* Python's long type corresponds to C's long long type.  */
#define builtin_type_pylong builtin_type (python_gdbarch)->builtin_long_long

/* Python's long type corresponds to C's long long type.  Unsigned version.  */
#define builtin_type_upylong builtin_type \
  (python_gdbarch)->builtin_unsigned_long_long

#define builtin_type_pybool \
  language_bool_type (python_language, python_gdbarch)

#define builtin_type_pychar \
  language_string_char_type (python_language, python_gdbarch)

typedef struct value_object {
  PyObject_HEAD
  struct value_object *next;
  struct value_object *prev;
  struct value *value;
  PyObject *address;
  PyObject *type;
  PyObject *dynamic_type;
} value_object;

/* List of all values which are currently exposed to Python. It is
   maintained so that when an objfile is discarded, preserve_values
   can copy the values' types if needed.  */
/* This variable is unnecessarily initialized to NULL in order to
   work around a linker bug on MacOS.  */
static value_object *values_in_python = NULL;

/* Called by the Python interpreter when deallocating a value object.  */
static void
valpy_dealloc (PyObject *obj)
{
  value_object *self = (value_object *) obj;

  /* Remove SELF from the global list.  */
  if (self->prev)
    self->prev->next = self->next;
  else
    {
      gdb_assert (values_in_python == self);
      values_in_python = self->next;
    }
  if (self->next)
    self->next->prev = self->prev;

  value_decref (self->value);

  Py_XDECREF (self->address);
  Py_XDECREF (self->type);
  Py_XDECREF (self->dynamic_type);

  Py_TYPE (self)->tp_free (self);
}

/* Helper to push a Value object on the global list.  */
static void
note_value (value_object *value_obj)
{
  value_obj->next = values_in_python;
  if (value_obj->next)
    value_obj->next->prev = value_obj;
  value_obj->prev = NULL;
  values_in_python = value_obj;
}

/* Convert a python object OBJ with type TYPE to a gdb value.  The
   python object in question must conform to the python buffer
   protocol.  On success, return the converted value, otherwise
   nullptr.  */

static struct value *
convert_buffer_and_type_to_value (PyObject *obj, struct type *type)
{
  Py_buffer_up buffer_up;
  Py_buffer py_buf;

  if (PyObject_CheckBuffer (obj) 
      && PyObject_GetBuffer (obj, &py_buf, PyBUF_SIMPLE) == 0)
    {
      /* Got a buffer, py_buf, out of obj.  Cause it to be released
	 when it goes out of scope.  */
      buffer_up.reset (&py_buf);
    }
  else
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Object must support the python buffer protocol."));
      return nullptr;
    }

  if (TYPE_LENGTH (type) > py_buf.len)
    {
      PyErr_SetString (PyExc_ValueError,
		       _("Size of type is larger than that of buffer object."));
      return nullptr;
    }

  return value_from_contents (type, (const gdb_byte *) py_buf.buf);
}

/* Called when a new gdb.Value object needs to be allocated.  Returns NULL on
   error, with a python exception set.  */
static PyObject *
valpy_new (PyTypeObject *subtype, PyObject *args, PyObject *kwargs)
{
  static const char *keywords[] = { "val", "type", NULL };
  PyObject *val_obj = nullptr;
  PyObject *type_obj = nullptr;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kwargs, "O|O", keywords,
					&val_obj, &type_obj))
    return nullptr;

  struct type *type = nullptr;

  if (type_obj != nullptr)
    {
      type = type_object_to_type (type_obj);
      if (type == nullptr)
	{
	  PyErr_SetString (PyExc_TypeError,
			   _("type argument must be a gdb.Type."));
	  return nullptr;
	}
    }

  value_object *value_obj = (value_object *) subtype->tp_alloc (subtype, 1);
  if (value_obj == NULL)
    {
      PyErr_SetString (PyExc_MemoryError, _("Could not allocate memory to "
					    "create Value object."));
      return NULL;
    }

  struct value *value;

  if (type == nullptr)
    value = convert_value_from_python (val_obj);
  else
    value = convert_buffer_and_type_to_value (val_obj, type);

  if (value == nullptr)
    {
      subtype->tp_free (value_obj);
      return NULL;
    }

  value_obj->value = release_value (value).release ();
  value_obj->address = NULL;
  value_obj->type = NULL;
  value_obj->dynamic_type = NULL;
  note_value (value_obj);

  return (PyObject *) value_obj;
}

/* Iterate over all the Value objects, calling preserve_one_value on
   each.  */
void
gdbpy_preserve_values (const struct extension_language_defn *extlang,
		       struct objfile *objfile, htab_t copied_types)
{
  value_object *iter;

  for (iter = values_in_python; iter; iter = iter->next)
    preserve_one_value (iter->value, objfile, copied_types);
}

/* Given a value of a pointer type, apply the C unary * operator to it.  */
static PyObject *
valpy_dereference (PyObject *self, PyObject *args)
{
  PyObject *result = NULL;

  try
    {
      struct value *res_val;
      scoped_value_mark free_values;

      res_val = value_ind (((value_object *) self)->value);
      result = value_to_value_object (res_val);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

/* Given a value of a pointer type or a reference type, return the value
   referenced. The difference between this function and valpy_dereference is
   that the latter applies * unary operator to a value, which need not always
   result in the value referenced. For example, for a value which is a reference
   to an 'int' pointer ('int *'), valpy_dereference will result in a value of
   type 'int' while valpy_referenced_value will result in a value of type
   'int *'.  */

static PyObject *
valpy_referenced_value (PyObject *self, PyObject *args)
{
  PyObject *result = NULL;

  try
    {
      struct value *self_val, *res_val;
      scoped_value_mark free_values;

      self_val = ((value_object *) self)->value;
      switch (check_typedef (value_type (self_val))->code ())
	{
	case TYPE_CODE_PTR:
	  res_val = value_ind (self_val);
	  break;
	case TYPE_CODE_REF:
	case TYPE_CODE_RVALUE_REF:
	  res_val = coerce_ref (self_val);
	  break;
	default:
	  error(_("Trying to get the referenced value from a value which is "
		  "neither a pointer nor a reference."));
	}

      result = value_to_value_object (res_val);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

/* Return a value which is a reference to the value.  */

static PyObject *
valpy_reference_value (PyObject *self, PyObject *args, enum type_code refcode)
{
  PyObject *result = NULL;

  try
    {
      struct value *self_val;
      scoped_value_mark free_values;

      self_val = ((value_object *) self)->value;
      result = value_to_value_object (value_ref (self_val, refcode));
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

static PyObject *
valpy_lvalue_reference_value (PyObject *self, PyObject *args)
{
  return valpy_reference_value (self, args, TYPE_CODE_REF);
}

static PyObject *
valpy_rvalue_reference_value (PyObject *self, PyObject *args)
{
  return valpy_reference_value (self, args, TYPE_CODE_RVALUE_REF);
}

/* Return a "const" qualified version of the value.  */

static PyObject *
valpy_const_value (PyObject *self, PyObject *args)
{
  PyObject *result = NULL;

  try
    {
      struct value *self_val, *res_val;
      scoped_value_mark free_values;

      self_val = ((value_object *) self)->value;
      res_val = make_cv_value (1, 0, self_val);
      result = value_to_value_object (res_val);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

/* Return "&value".  */
static PyObject *
valpy_get_address (PyObject *self, void *closure)
{
  value_object *val_obj = (value_object *) self;

  if (!val_obj->address)
    {
      try
	{
	  struct value *res_val;
	  scoped_value_mark free_values;

	  res_val = value_addr (val_obj->value);
	  val_obj->address = value_to_value_object (res_val);
	}
      catch (const gdb_exception &except)
	{
	  val_obj->address = Py_None;
	  Py_INCREF (Py_None);
	}
    }

  Py_XINCREF (val_obj->address);

  return val_obj->address;
}

/* Return type of the value.  */
static PyObject *
valpy_get_type (PyObject *self, void *closure)
{
  value_object *obj = (value_object *) self;

  if (!obj->type)
    {
      obj->type = type_to_type_object (value_type (obj->value));
      if (!obj->type)
	return NULL;
    }
  Py_INCREF (obj->type);
  return obj->type;
}

/* Return dynamic type of the value.  */

static PyObject *
valpy_get_dynamic_type (PyObject *self, void *closure)
{
  value_object *obj = (value_object *) self;
  struct type *type = NULL;

  if (obj->dynamic_type != NULL)
    {
      Py_INCREF (obj->dynamic_type);
      return obj->dynamic_type;
    }

  try
    {
      struct value *val = obj->value;
      scoped_value_mark free_values;

      type = value_type (val);
      type = check_typedef (type);

      if (((type->code () == TYPE_CODE_PTR) || TYPE_IS_REFERENCE (type))
	  && (TYPE_TARGET_TYPE (type)->code () == TYPE_CODE_STRUCT))
	{
	  struct value *target;
	  int was_pointer = type->code () == TYPE_CODE_PTR;

	  if (was_pointer)
	    target = value_ind (val);
	  else
	    target = coerce_ref (val);
	  type = value_rtti_type (target, NULL, NULL, NULL);

	  if (type)
	    {
	      if (was_pointer)
		type = lookup_pointer_type (type);
	      else
		type = lookup_lvalue_reference_type (type);
	    }
	}
      else if (type->code () == TYPE_CODE_STRUCT)
	type = value_rtti_type (val, NULL, NULL, NULL);
      else
	{
	  /* Re-use object's static type.  */
	  type = NULL;
	}
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (type == NULL)
    obj->dynamic_type = valpy_get_type (self, NULL);
  else
    obj->dynamic_type = type_to_type_object (type);

  Py_XINCREF (obj->dynamic_type);
  return obj->dynamic_type;
}

/* Implementation of gdb.Value.lazy_string ([encoding] [, length]) ->
   string.  Return a PyObject representing a lazy_string_object type.
   A lazy string is a pointer to a string with an optional encoding and
   length.  If ENCODING is not given, encoding is set to None.  If an
   ENCODING is provided the encoding parameter is set to ENCODING, but
   the string is not encoded.
   If LENGTH is provided then the length parameter is set to LENGTH.
   Otherwise if the value is an array of known length then the array's length
   is used.  Otherwise the length will be set to -1 (meaning first null of
   appropriate with).

   Note: In order to not break any existing uses this allows creating
   lazy strings from anything.  PR 20769.  E.g.,
   gdb.parse_and_eval("my_int_variable").lazy_string().
   "It's easier to relax restrictions than it is to impose them after the
   fact."  So we should be flagging any unintended uses as errors, but it's
   perhaps too late for that.  */

static PyObject *
valpy_lazy_string (PyObject *self, PyObject *args, PyObject *kw)
{
  gdb_py_longest length = -1;
  struct value *value = ((value_object *) self)->value;
  const char *user_encoding = NULL;
  static const char *keywords[] = { "encoding", "length", NULL };
  PyObject *str_obj = NULL;

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "|s" GDB_PY_LL_ARG,
					keywords, &user_encoding, &length))
    return NULL;

  if (length < -1)
    {
      PyErr_SetString (PyExc_ValueError, _("Invalid length."));
      return NULL;
    }

  try
    {
      scoped_value_mark free_values;
      struct type *type, *realtype;
      CORE_ADDR addr;

      type = value_type (value);
      realtype = check_typedef (type);

      switch (realtype->code ())
	{
	case TYPE_CODE_ARRAY:
	  {
	    LONGEST array_length = -1;
	    LONGEST low_bound, high_bound;

	    /* PR 20786: There's no way to specify an array of length zero.
	       Record a length of [0,-1] which is how Ada does it.  Anything
	       we do is broken, but this one possible solution.  */
	    if (get_array_bounds (realtype, &low_bound, &high_bound))
	      array_length = high_bound - low_bound + 1;
	    if (length == -1)
	      length = array_length;
	    else if (array_length == -1)
	      {
		type = lookup_array_range_type (TYPE_TARGET_TYPE (realtype),
						0, length - 1);
	      }
	    else if (length != array_length)
	      {
		/* We need to create a new array type with the
		   specified length.  */
		if (length > array_length)
		  error (_("Length is larger than array size."));
		type = lookup_array_range_type (TYPE_TARGET_TYPE (realtype),
						low_bound,
						low_bound + length - 1);
	      }
	    addr = value_address (value);
	    break;
	  }
	case TYPE_CODE_PTR:
	  /* If a length is specified we defer creating an array of the
	     specified width until we need to.  */
	  addr = value_as_address (value);
	  break;
	default:
	  /* Should flag an error here.  PR 20769.  */
	  addr = value_address (value);
	  break;
	}

      str_obj = gdbpy_create_lazy_string_object (addr, length, user_encoding,
						 type);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return str_obj;
}

/* Implementation of gdb.Value.string ([encoding] [, errors]
   [, length]) -> string.  Return Unicode string with value contents.
   If ENCODING is not given, the string is assumed to be encoded in
   the target's charset.  If LENGTH is provided, only fetch string to
   the length provided.  */

static PyObject *
valpy_string (PyObject *self, PyObject *args, PyObject *kw)
{
  int length = -1;
  gdb::unique_xmalloc_ptr<gdb_byte> buffer;
  struct value *value = ((value_object *) self)->value;
  const char *encoding = NULL;
  const char *errors = NULL;
  const char *user_encoding = NULL;
  const char *la_encoding = NULL;
  struct type *char_type;
  static const char *keywords[] = { "encoding", "errors", "length", NULL };

  if (!gdb_PyArg_ParseTupleAndKeywords (args, kw, "|ssi", keywords,
					&user_encoding, &errors, &length))
    return NULL;

  try
    {
      c_get_string (value, &buffer, &length, &char_type, &la_encoding);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  encoding = (user_encoding && *user_encoding) ? user_encoding : la_encoding;
  return PyUnicode_Decode ((const char *) buffer.get (),
			   length * TYPE_LENGTH (char_type),
			   encoding, errors);
}

/* Given a Python object, copy its truth value to a C bool (the value
   pointed by dest).
   If src_obj is NULL, then *dest is not modified.

   Return true in case of success (including src_obj being NULL), false
   in case of error.  */

static bool
copy_py_bool_obj (bool *dest, PyObject *src_obj)
{
  if (src_obj)
    {
      int cmp = PyObject_IsTrue (src_obj);
      if (cmp < 0)
	return false;
      *dest = cmp;
    }

  return true;
}

/* Implementation of gdb.Value.format_string (...) -> string.
   Return Unicode string with value contents formatted using the
   keyword-only arguments.  */

static PyObject *
valpy_format_string (PyObject *self, PyObject *args, PyObject *kw)
{
  static const char *keywords[] =
    {
      /* Basic C/C++ options.  */
      "raw",			/* See the /r option to print.  */
      "pretty_arrays",		/* See set print array on|off.  */
      "pretty_structs",		/* See set print pretty on|off.  */
      "array_indexes",		/* See set print array-indexes on|off.  */
      "symbols",		/* See set print symbol on|off.  */
      "unions",			/* See set print union on|off.  */
      /* C++ options.  */
      "deref_refs",		/* No corresponding setting.  */
      "actual_objects",		/* See set print object on|off.  */
      "static_members",		/* See set print static-members on|off.  */
      /* C non-bool options.  */
      "max_elements", 		/* See set print elements N.  */
      "max_depth",		/* See set print max-depth N.  */
      "repeat_threshold",	/* See set print repeats.  */
      "format",			/* The format passed to the print command.  */
      NULL
    };

  /* This function has too many arguments to be useful as positionals, so
     the user should specify them all as keyword arguments.
     Python 3.3 and later have a way to specify it (both in C and Python
     itself), but we could be compiled with older versions, so we just
     check that the args tuple is empty.  */
  Py_ssize_t positional_count = PyObject_Length (args);
  if (positional_count < 0)
    return NULL;
  else if (positional_count > 0)
    {
      /* This matches the error message that Python 3.3 raises when
	 passing positionals to functions expecting keyword-only
	 arguments.  */
      PyErr_Format (PyExc_TypeError,
		    "format_string() takes 0 positional arguments but %zu were given",
		    positional_count);
      return NULL;
    }

  struct value_print_options opts;
  get_user_print_options (&opts);
  opts.deref_ref = 0;

  /* We need objects for booleans as the "p" flag for bools is new in
     Python 3.3.  */
  PyObject *raw_obj = NULL;
  PyObject *pretty_arrays_obj = NULL;
  PyObject *pretty_structs_obj = NULL;
  PyObject *array_indexes_obj = NULL;
  PyObject *symbols_obj = NULL;
  PyObject *unions_obj = NULL;
  PyObject *deref_refs_obj = NULL;
  PyObject *actual_objects_obj = NULL;
  PyObject *static_members_obj = NULL;
  char *format = NULL;
  if (!gdb_PyArg_ParseTupleAndKeywords (args,
					kw,
					"|O!O!O!O!O!O!O!O!O!IIIs",
					keywords,
					&PyBool_Type, &raw_obj,
					&PyBool_Type, &pretty_arrays_obj,
					&PyBool_Type, &pretty_structs_obj,
					&PyBool_Type, &array_indexes_obj,
					&PyBool_Type, &symbols_obj,
					&PyBool_Type, &unions_obj,
					&PyBool_Type, &deref_refs_obj,
					&PyBool_Type, &actual_objects_obj,
					&PyBool_Type, &static_members_obj,
					&opts.print_max,
					&opts.max_depth,
					&opts.repeat_count_threshold,
					&format))
    return NULL;

  /* Set boolean arguments.  */
  if (!copy_py_bool_obj (&opts.raw, raw_obj))
    return NULL;
  if (!copy_py_bool_obj (&opts.prettyformat_arrays, pretty_arrays_obj))
    return NULL;
  if (!copy_py_bool_obj (&opts.prettyformat_structs, pretty_structs_obj))
    return NULL;
  if (!copy_py_bool_obj (&opts.print_array_indexes, array_indexes_obj))
    return NULL;
  if (!copy_py_bool_obj (&opts.symbol_print, symbols_obj))
    return NULL;
  if (!copy_py_bool_obj (&opts.unionprint, unions_obj))
    return NULL;
  if (!copy_py_bool_obj (&opts.deref_ref, deref_refs_obj))
    return NULL;
  if (!copy_py_bool_obj (&opts.objectprint, actual_objects_obj))
    return NULL;
  if (!copy_py_bool_obj (&opts.static_field_print, static_members_obj))
    return NULL;

  /* Numeric arguments for which 0 means unlimited (which we represent as
     UINT_MAX).  Note that the max-depth numeric argument uses -1 as
     unlimited, and 0 is a valid choice.  */
  if (opts.print_max == 0)
    opts.print_max = UINT_MAX;
  if (opts.repeat_count_threshold == 0)
    opts.repeat_count_threshold = UINT_MAX;

  /* Other arguments.  */
  if (format != NULL)
    {
      if (strlen (format) == 1)
	opts.format = format[0];
      else
	{
	  /* Mimic the message on standard Python ones for similar
	     errors.  */
	  PyErr_SetString (PyExc_ValueError,
			   "a single character is required");
	  return NULL;
	}
    }

  string_file stb;

  try
    {
      common_val_print (((value_object *) self)->value, &stb, 0,
			&opts, python_language);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return PyUnicode_Decode (stb.c_str (), stb.size (), host_charset (), NULL);
}

/* A helper function that implements the various cast operators.  */

static PyObject *
valpy_do_cast (PyObject *self, PyObject *args, enum exp_opcode op)
{
  PyObject *type_obj, *result = NULL;
  struct type *type;

  if (! PyArg_ParseTuple (args, "O", &type_obj))
    return NULL;

  type = type_object_to_type (type_obj);
  if (! type)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Argument must be a type."));
      return NULL;
    }

  try
    {
      struct value *val = ((value_object *) self)->value;
      struct value *res_val;
      scoped_value_mark free_values;

      if (op == UNOP_DYNAMIC_CAST)
	res_val = value_dynamic_cast (type, val);
      else if (op == UNOP_REINTERPRET_CAST)
	res_val = value_reinterpret_cast (type, val);
      else
	{
	  gdb_assert (op == UNOP_CAST);
	  res_val = value_cast (type, val);
	}

      result = value_to_value_object (res_val);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

/* Implementation of the "cast" method.  */

static PyObject *
valpy_cast (PyObject *self, PyObject *args)
{
  return valpy_do_cast (self, args, UNOP_CAST);
}

/* Implementation of the "dynamic_cast" method.  */

static PyObject *
valpy_dynamic_cast (PyObject *self, PyObject *args)
{
  return valpy_do_cast (self, args, UNOP_DYNAMIC_CAST);
}

/* Implementation of the "reinterpret_cast" method.  */

static PyObject *
valpy_reinterpret_cast (PyObject *self, PyObject *args)
{
  return valpy_do_cast (self, args, UNOP_REINTERPRET_CAST);
}

static Py_ssize_t
valpy_length (PyObject *self)
{
  /* We don't support getting the number of elements in a struct / class.  */
  PyErr_SetString (PyExc_NotImplementedError,
		   _("Invalid operation on gdb.Value."));
  return -1;
}

/* Return 1 if the gdb.Field object FIELD is present in the value V.
   Returns 0 otherwise.  If any Python error occurs, -1 is returned.  */

static int
value_has_field (struct value *v, PyObject *field)
{
  struct type *parent_type, *val_type;
  enum type_code type_code;
  gdbpy_ref<> type_object (PyObject_GetAttrString (field, "parent_type"));
  int has_field = 0;

  if (type_object == NULL)
    return -1;

  parent_type = type_object_to_type (type_object.get ());
  if (parent_type == NULL)
    {
      PyErr_SetString (PyExc_TypeError,
		       _("'parent_type' attribute of gdb.Field object is not a"
			 "gdb.Type object."));
      return -1;
    }

  try
    {
      val_type = value_type (v);
      val_type = check_typedef (val_type);
      if (TYPE_IS_REFERENCE (val_type) || val_type->code () == TYPE_CODE_PTR)
	val_type = check_typedef (TYPE_TARGET_TYPE (val_type));

      type_code = val_type->code ();
      if ((type_code == TYPE_CODE_STRUCT || type_code == TYPE_CODE_UNION)
	  && types_equal (val_type, parent_type))
	has_field = 1;
      else
	has_field = 0;
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_SET_HANDLE_EXCEPTION (except);
    }

  return has_field;
}

/* Return the value of a flag FLAG_NAME in a gdb.Field object FIELD.
   Returns 1 if the flag value is true, 0 if it is false, and -1 if
   a Python error occurs.  */

static int
get_field_flag (PyObject *field, const char *flag_name)
{
  gdbpy_ref<> flag_object (PyObject_GetAttrString (field, flag_name));

  if (flag_object == NULL)
    return -1;

  return PyObject_IsTrue (flag_object.get ());
}

/* Return the "type" attribute of a gdb.Field object.
   Returns NULL on error, with a Python exception set.  */

static struct type *
get_field_type (PyObject *field)
{
  gdbpy_ref<> ftype_obj (PyObject_GetAttrString (field, "type"));
  struct type *ftype;

  if (ftype_obj == NULL)
    return NULL;
  ftype = type_object_to_type (ftype_obj.get ());
  if (ftype == NULL)
    PyErr_SetString (PyExc_TypeError,
		     _("'type' attribute of gdb.Field object is not a "
		       "gdb.Type object."));

  return ftype;
}

/* Given string name or a gdb.Field object corresponding to an element inside
   a structure, return its value object.  Returns NULL on error, with a python
   exception set.  */

static PyObject *
valpy_getitem (PyObject *self, PyObject *key)
{
  struct gdb_exception except;
  value_object *self_value = (value_object *) self;
  gdb::unique_xmalloc_ptr<char> field;
  struct type *base_class_type = NULL, *field_type = NULL;
  long bitpos = -1;
  PyObject *result = NULL;

  if (gdbpy_is_string (key))
    {
      field = python_string_to_host_string (key);
      if (field == NULL)
	return NULL;
    }
  else if (gdbpy_is_field (key))
    {
      int is_base_class, valid_field;

      valid_field = value_has_field (self_value->value, key);
      if (valid_field < 0)
	return NULL;
      else if (valid_field == 0)
	{
	  PyErr_SetString (PyExc_TypeError,
			   _("Invalid lookup for a field not contained in "
			     "the value."));

	  return NULL;
	}

      is_base_class = get_field_flag (key, "is_base_class");
      if (is_base_class < 0)
	return NULL;
      else if (is_base_class > 0)
	{
	  base_class_type = get_field_type (key);
	  if (base_class_type == NULL)
	    return NULL;
	}
      else
	{
	  gdbpy_ref<> name_obj (PyObject_GetAttrString (key, "name"));

	  if (name_obj == NULL)
	    return NULL;

	  if (name_obj != Py_None)
	    {
	      field = python_string_to_host_string (name_obj.get ());
	      if (field == NULL)
		return NULL;
	    }
	  else
	    {
	      if (!PyObject_HasAttrString (key, "bitpos"))
		{
		  PyErr_SetString (PyExc_AttributeError,
				   _("gdb.Field object has no name and no "
				     "'bitpos' attribute."));

		  return NULL;
		}
	      gdbpy_ref<> bitpos_obj (PyObject_GetAttrString (key, "bitpos"));
	      if (bitpos_obj == NULL)
		return NULL;
	      if (!gdb_py_int_as_long (bitpos_obj.get (), &bitpos))
		return NULL;

	      field_type = get_field_type (key);
	      if (field_type == NULL)
		return NULL;
	    }
	}
    }

  try
    {
      struct value *tmp = self_value->value;
      struct value *res_val = NULL;
      scoped_value_mark free_values;

      if (field)
	res_val = value_struct_elt (&tmp, NULL, field.get (), NULL,
				    "struct/class/union");
      else if (bitpos >= 0)
	res_val = value_struct_elt_bitpos (&tmp, bitpos, field_type,
					   "struct/class/union");
      else if (base_class_type != NULL)
	{
	  struct type *val_type;

	  val_type = check_typedef (value_type (tmp));
	  if (val_type->code () == TYPE_CODE_PTR)
	    res_val = value_cast (lookup_pointer_type (base_class_type), tmp);
	  else if (val_type->code () == TYPE_CODE_REF)
	    res_val = value_cast (lookup_lvalue_reference_type (base_class_type),
				  tmp);
	  else if (val_type->code () == TYPE_CODE_RVALUE_REF)
	    res_val = value_cast (lookup_rvalue_reference_type (base_class_type),
				  tmp);
	  else
	    res_val = value_cast (base_class_type, tmp);
	}
      else
	{
	  /* Assume we are attempting an array access, and let the
	     value code throw an exception if the index has an invalid
	     type.  */
	  struct value *idx = convert_value_from_python (key);

	  if (idx != NULL)
	    {
	      /* Check the value's type is something that can be accessed via
		 a subscript.  */
	      struct type *type;

	      tmp = coerce_ref (tmp);
	      type = check_typedef (value_type (tmp));
	      if (type->code () != TYPE_CODE_ARRAY
		  && type->code () != TYPE_CODE_PTR)
		  error (_("Cannot subscript requested type."));
	      else
		res_val = value_subscript (tmp, value_as_long (idx));
	    }
	}

      if (res_val)
	result = value_to_value_object (res_val);
    }
  catch (gdb_exception &ex)
    {
      except = std::move (ex);
    }

  GDB_PY_HANDLE_EXCEPTION (except);

  return result;
}

static int
valpy_setitem (PyObject *self, PyObject *key, PyObject *value)
{
  PyErr_Format (PyExc_NotImplementedError,
		_("Setting of struct elements is not currently supported."));
  return -1;
}

/* Called by the Python interpreter to perform an inferior function
   call on the value.  Returns NULL on error, with a python exception set.  */
static PyObject *
valpy_call (PyObject *self, PyObject *args, PyObject *keywords)
{
  Py_ssize_t args_count;
  struct value *function = ((value_object *) self)->value;
  struct value **vargs = NULL;
  struct type *ftype = NULL;
  PyObject *result = NULL;

  try
    {
      ftype = check_typedef (value_type (function));
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (ftype->code () != TYPE_CODE_FUNC)
    {
      PyErr_SetString (PyExc_RuntimeError,
		       _("Value is not callable (not TYPE_CODE_FUNC)."));
      return NULL;
    }

  if (! PyTuple_Check (args))
    {
      PyErr_SetString (PyExc_TypeError,
		       _("Inferior arguments must be provided in a tuple."));
      return NULL;
    }

  args_count = PyTuple_Size (args);
  if (args_count > 0)
    {
      int i;

      vargs = XALLOCAVEC (struct value *, args_count);
      for (i = 0; i < args_count; i++)
	{
	  PyObject *item = PyTuple_GetItem (args, i);

	  if (item == NULL)
	    return NULL;

	  vargs[i] = convert_value_from_python (item);
	  if (vargs[i] == NULL)
	    return NULL;
	}
    }

  try
    {
      scoped_value_mark free_values;

      value *return_value
	= call_function_by_hand (function, NULL,
				 gdb::make_array_view (vargs, args_count));
      result = value_to_value_object (return_value);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

/* Called by the Python interpreter to obtain string representation
   of the object.  */
static PyObject *
valpy_str (PyObject *self)
{
  struct value_print_options opts;

  get_user_print_options (&opts);
  opts.deref_ref = 0;

  string_file stb;

  try
    {
      common_val_print (((value_object *) self)->value, &stb, 0,
			&opts, python_language);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return PyUnicode_Decode (stb.c_str (), stb.size (), host_charset (), NULL);
}

/* Implements gdb.Value.is_optimized_out.  */
static PyObject *
valpy_get_is_optimized_out (PyObject *self, void *closure)
{
  struct value *value = ((value_object *) self)->value;
  int opt = 0;

  try
    {
      opt = value_optimized_out (value);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (opt)
    Py_RETURN_TRUE;

  Py_RETURN_FALSE;
}

/* Implements gdb.Value.is_lazy.  */
static PyObject *
valpy_get_is_lazy (PyObject *self, void *closure)
{
  struct value *value = ((value_object *) self)->value;
  int opt = 0;

  try
    {
      opt = value_lazy (value);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (opt)
    Py_RETURN_TRUE;

  Py_RETURN_FALSE;
}

/* Implements gdb.Value.fetch_lazy ().  */
static PyObject *
valpy_fetch_lazy (PyObject *self, PyObject *args)
{
  struct value *value = ((value_object *) self)->value;

  try
    {
      if (value_lazy (value))
	value_fetch_lazy (value);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  Py_RETURN_NONE;
}

/* Calculate and return the address of the PyObject as the value of
   the builtin __hash__ call.  */
static Py_hash_t
valpy_hash (PyObject *self)
{
  return (intptr_t) self;
}

enum valpy_opcode
{
  VALPY_ADD,
  VALPY_SUB,
  VALPY_MUL,
  VALPY_DIV,
  VALPY_REM,
  VALPY_POW,
  VALPY_LSH,
  VALPY_RSH,
  VALPY_BITAND,
  VALPY_BITOR,
  VALPY_BITXOR
};

/* If TYPE is a reference, return the target; otherwise return TYPE.  */
#define STRIP_REFERENCE(TYPE) \
  (TYPE_IS_REFERENCE (TYPE) ? (TYPE_TARGET_TYPE (TYPE)) : (TYPE))

/* Helper for valpy_binop.  Returns a value object which is the result
   of applying the operation specified by OPCODE to the given
   arguments.  Throws a GDB exception on error.  */

static PyObject *
valpy_binop_throw (enum valpy_opcode opcode, PyObject *self, PyObject *other)
{
  PyObject *result = NULL;

  struct value *arg1, *arg2;
  struct value *res_val = NULL;
  enum exp_opcode op = OP_NULL;
  int handled = 0;

  scoped_value_mark free_values;

  /* If the gdb.Value object is the second operand, then it will be
     passed to us as the OTHER argument, and SELF will be an entirely
     different kind of object, altogether.  Because of this, we can't
     assume self is a gdb.Value object and need to convert it from
     python as well.  */
  arg1 = convert_value_from_python (self);
  if (arg1 == NULL)
    return NULL;

  arg2 = convert_value_from_python (other);
  if (arg2 == NULL)
    return NULL;

  switch (opcode)
    {
    case VALPY_ADD:
      {
	struct type *ltype = value_type (arg1);
	struct type *rtype = value_type (arg2);

	ltype = check_typedef (ltype);
	ltype = STRIP_REFERENCE (ltype);
	rtype = check_typedef (rtype);
	rtype = STRIP_REFERENCE (rtype);

	handled = 1;
	if (ltype->code () == TYPE_CODE_PTR
	    && is_integral_type (rtype))
	  res_val = value_ptradd (arg1, value_as_long (arg2));
	else if (rtype->code () == TYPE_CODE_PTR
		 && is_integral_type (ltype))
	  res_val = value_ptradd (arg2, value_as_long (arg1));
	else
	  {
	    handled = 0;
	    op = BINOP_ADD;
	  }
      }
      break;
    case VALPY_SUB:
      {
	struct type *ltype = value_type (arg1);
	struct type *rtype = value_type (arg2);

	ltype = check_typedef (ltype);
	ltype = STRIP_REFERENCE (ltype);
	rtype = check_typedef (rtype);
	rtype = STRIP_REFERENCE (rtype);

	handled = 1;
	if (ltype->code () == TYPE_CODE_PTR
	    && rtype->code () == TYPE_CODE_PTR)
	  /* A ptrdiff_t for the target would be preferable here.  */
	  res_val = value_from_longest (builtin_type_pyint,
					value_ptrdiff (arg1, arg2));
	else if (ltype->code () == TYPE_CODE_PTR
		 && is_integral_type (rtype))
	  res_val = value_ptradd (arg1, - value_as_long (arg2));
	else
	  {
	    handled = 0;
	    op = BINOP_SUB;
	  }
      }
      break;
    case VALPY_MUL:
      op = BINOP_MUL;
      break;
    case VALPY_DIV:
      op = BINOP_DIV;
      break;
    case VALPY_REM:
      op = BINOP_REM;
      break;
    case VALPY_POW:
      op = BINOP_EXP;
      break;
    case VALPY_LSH:
      op = BINOP_LSH;
      break;
    case VALPY_RSH:
      op = BINOP_RSH;
      break;
    case VALPY_BITAND:
      op = BINOP_BITWISE_AND;
      break;
    case VALPY_BITOR:
      op = BINOP_BITWISE_IOR;
      break;
    case VALPY_BITXOR:
      op = BINOP_BITWISE_XOR;
      break;
    }

  if (!handled)
    {
      if (binop_user_defined_p (op, arg1, arg2))
	res_val = value_x_binop (arg1, arg2, op, OP_NULL, EVAL_NORMAL);
      else
	res_val = value_binop (arg1, arg2, op);
    }

  if (res_val)
    result = value_to_value_object (res_val);

  return result;
}

/* Returns a value object which is the result of applying the operation
   specified by OPCODE to the given arguments.  Returns NULL on error, with
   a python exception set.  */
static PyObject *
valpy_binop (enum valpy_opcode opcode, PyObject *self, PyObject *other)
{
  PyObject *result = NULL;

  try
    {
      result = valpy_binop_throw (opcode, self, other);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

static PyObject *
valpy_add (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_ADD, self, other);
}

static PyObject *
valpy_subtract (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_SUB, self, other);
}

static PyObject *
valpy_multiply (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_MUL, self, other);
}

static PyObject *
valpy_divide (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_DIV, self, other);
}

static PyObject *
valpy_remainder (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_REM, self, other);
}

static PyObject *
valpy_power (PyObject *self, PyObject *other, PyObject *unused)
{
  /* We don't support the ternary form of pow.  I don't know how to express
     that, so let's just throw NotImplementedError to at least do something
     about it.  */
  if (unused != Py_None)
    {
      PyErr_SetString (PyExc_NotImplementedError,
		       "Invalid operation on gdb.Value.");
      return NULL;
    }

  return valpy_binop (VALPY_POW, self, other);
}

static PyObject *
valpy_negative (PyObject *self)
{
  PyObject *result = NULL;

  try
    {
      /* Perhaps overkill, but consistency has some virtue.  */
      scoped_value_mark free_values;
      struct value *val;

      val = value_neg (((value_object *) self)->value);
      result = value_to_value_object (val);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return result;
}

static PyObject *
valpy_positive (PyObject *self)
{
  return value_to_value_object (((value_object *) self)->value);
}

static PyObject *
valpy_absolute (PyObject *self)
{
  struct value *value = ((value_object *) self)->value;
  int isabs = 1;

  try
    {
      scoped_value_mark free_values;

      if (value_less (value, value_zero (value_type (value), not_lval)))
	isabs = 0;
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (isabs)
    return valpy_positive (self);
  else
    return valpy_negative (self);
}

/* Implements boolean evaluation of gdb.Value.  */
static int
valpy_nonzero (PyObject *self)
{
  struct gdb_exception except;
  value_object *self_value = (value_object *) self;
  struct type *type;
  int nonzero = 0; /* Appease GCC warning.  */

  try
    {
      type = check_typedef (value_type (self_value->value));

      if (is_integral_type (type) || type->code () == TYPE_CODE_PTR)
	nonzero = !!value_as_long (self_value->value);
      else if (is_floating_value (self_value->value))
	nonzero = !target_float_is_zero (value_contents (self_value->value),
					 type);
      else
	/* All other values are True.  */
	nonzero = 1;
    }
  catch (gdb_exception &ex)
    {
      except = std::move (ex);
    }

  /* This is not documented in the Python documentation, but if this
     function fails, return -1 as slot_nb_nonzero does (the default
     Python nonzero function).  */
  GDB_PY_SET_HANDLE_EXCEPTION (except);

  return nonzero;
}

/* Implements ~ for value objects.  */
static PyObject *
valpy_invert (PyObject *self)
{
  struct value *val = NULL;

  try
    {
      val = value_complement (((value_object *) self)->value);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return value_to_value_object (val);
}

/* Implements left shift for value objects.  */
static PyObject *
valpy_lsh (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_LSH, self, other);
}

/* Implements right shift for value objects.  */
static PyObject *
valpy_rsh (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_RSH, self, other);
}

/* Implements bitwise and for value objects.  */
static PyObject *
valpy_and (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_BITAND, self, other);
}

/* Implements bitwise or for value objects.  */
static PyObject *
valpy_or (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_BITOR, self, other);
}

/* Implements bitwise xor for value objects.  */
static PyObject *
valpy_xor (PyObject *self, PyObject *other)
{
  return valpy_binop (VALPY_BITXOR, self, other);
}

/* Helper for valpy_richcompare.  Implements comparison operations for
   value objects.  Returns true/false on success.  Returns -1 with a
   Python exception set if a Python error is detected.  Throws a GDB
   exception on other errors (memory error, etc.).  */

static int
valpy_richcompare_throw (PyObject *self, PyObject *other, int op)
{
  int result;
  struct value *value_other;
  struct value *value_self;

  scoped_value_mark free_values;

  value_other = convert_value_from_python (other);
  if (value_other == NULL)
    return -1;

  value_self = ((value_object *) self)->value;

  switch (op)
    {
    case Py_LT:
      result = value_less (value_self, value_other);
      break;
    case Py_LE:
      result = value_less (value_self, value_other)
	|| value_equal (value_self, value_other);
      break;
    case Py_EQ:
      result = value_equal (value_self, value_other);
      break;
    case Py_NE:
      result = !value_equal (value_self, value_other);
      break;
    case Py_GT:
      result = value_less (value_other, value_self);
      break;
    case Py_GE:
      result = (value_less (value_other, value_self)
		|| value_equal (value_self, value_other));
      break;
    default:
      /* Can't happen.  */
      PyErr_SetString (PyExc_NotImplementedError,
		       _("Invalid operation on gdb.Value."));
      result = -1;
      break;
    }

  return result;
}


/* Implements comparison operations for value objects.  Returns NULL on error,
   with a python exception set.  */
static PyObject *
valpy_richcompare (PyObject *self, PyObject *other, int op)
{
  int result = 0;

  if (other == Py_None)
    /* Comparing with None is special.  From what I can tell, in Python
       None is smaller than anything else.  */
    switch (op) {
      case Py_LT:
      case Py_LE:
      case Py_EQ:
	Py_RETURN_FALSE;
      case Py_NE:
      case Py_GT:
      case Py_GE:
	Py_RETURN_TRUE;
      default:
	/* Can't happen.  */
	PyErr_SetString (PyExc_NotImplementedError,
			 _("Invalid operation on gdb.Value."));
	return NULL;
    }

  try
    {
      result = valpy_richcompare_throw (self, other, op);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  /* In this case, the Python exception has already been set.  */
  if (result < 0)
    return NULL;

  if (result == 1)
    Py_RETURN_TRUE;

  Py_RETURN_FALSE;
}

#ifndef IS_PY3K
/* Implements conversion to int.  */
static PyObject *
valpy_int (PyObject *self)
{
  struct value *value = ((value_object *) self)->value;
  struct type *type = value_type (value);
  LONGEST l = 0;

  try
    {
      if (is_floating_value (value))
	{
	  type = builtin_type_pylong;
	  value = value_cast (type, value);
	}

      if (!is_integral_type (type)
	  && type->code () != TYPE_CODE_PTR)
	error (_("Cannot convert value to int."));

      l = value_as_long (value);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (type->is_unsigned ())
    return gdb_py_object_from_ulongest (l).release ();
  else
    return gdb_py_object_from_longest (l).release ();
}
#endif

/* Implements conversion to long.  */
static PyObject *
valpy_long (PyObject *self)
{
  struct value *value = ((value_object *) self)->value;
  struct type *type = value_type (value);
  LONGEST l = 0;

  try
    {
      if (is_floating_value (value))
	{
	  type = builtin_type_pylong;
	  value = value_cast (type, value);
	}

      type = check_typedef (type);

      if (!is_integral_type (type)
	  && type->code () != TYPE_CODE_PTR)
	error (_("Cannot convert value to long."));

      l = value_as_long (value);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (type->is_unsigned ())
    return gdb_py_object_from_ulongest (l).release ();
  else
    return gdb_py_object_from_longest (l).release ();
}

/* Implements conversion to float.  */
static PyObject *
valpy_float (PyObject *self)
{
  struct value *value = ((value_object *) self)->value;
  struct type *type = value_type (value);
  double d = 0;

  try
    {
      type = check_typedef (type);

      if (type->code () == TYPE_CODE_FLT && is_floating_value (value))
	d = target_float_to_host_double (value_contents (value), type);
      else if (type->code () == TYPE_CODE_INT)
	{
	  /* Note that valpy_long accepts TYPE_CODE_PTR and some
	     others here here -- but casting a pointer or bool to a
	     float seems wrong.  */
	  d = value_as_long (value);
	}
      else
	error (_("Cannot convert value to float."));
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return PyFloat_FromDouble (d);
}

/* Returns an object for a value which is released from the all_values chain,
   so its lifetime is not bound to the execution of a command.  */
PyObject *
value_to_value_object (struct value *val)
{
  value_object *val_obj;

  val_obj = PyObject_New (value_object, &value_object_type);
  if (val_obj != NULL)
    {
      val_obj->value = release_value (val).release ();
      val_obj->address = NULL;
      val_obj->type = NULL;
      val_obj->dynamic_type = NULL;
      note_value (val_obj);
    }

  return (PyObject *) val_obj;
}

/* Returns an object for a value, but without releasing it from the
   all_values chain.  */
PyObject *
value_to_value_object_no_release (struct value *val)
{
  value_object *val_obj;

  val_obj = PyObject_New (value_object, &value_object_type);
  if (val_obj != NULL)
    {
      value_incref (val);
      val_obj->value = val;
      val_obj->address = NULL;
      val_obj->type = NULL;
      val_obj->dynamic_type = NULL;
      note_value (val_obj);
    }

  return (PyObject *) val_obj;
}

/* Returns a borrowed reference to the struct value corresponding to
   the given value object.  */
struct value *
value_object_to_value (PyObject *self)
{
  value_object *real;

  if (! PyObject_TypeCheck (self, &value_object_type))
    return NULL;
  real = (value_object *) self;
  return real->value;
}

/* Try to convert a Python value to a gdb value.  If the value cannot
   be converted, set a Python exception and return NULL.  Returns a
   reference to a new value on the all_values chain.  */

struct value *
convert_value_from_python (PyObject *obj)
{
  struct value *value = NULL; /* -Wall */
  int cmp;

  gdb_assert (obj != NULL);

  try
    {
      if (PyBool_Check (obj))
	{
	  cmp = PyObject_IsTrue (obj);
	  if (cmp >= 0)
	    value = value_from_longest (builtin_type_pybool, cmp);
	}
      /* Make a long logic check first.  In Python 3.x, internally,
	 all integers are represented as longs.  In Python 2.x, there
	 is still a differentiation internally between a PyInt and a
	 PyLong.  Explicitly do this long check conversion first. In
	 GDB, for Python 3.x, we #ifdef PyInt = PyLong.  This check has
	 to be done first to ensure we do not lose information in the
	 conversion process.  */
      else if (PyLong_Check (obj))
	{
	  LONGEST l = PyLong_AsLongLong (obj);

	  if (PyErr_Occurred ())
	    {
	      /* If the error was an overflow, we can try converting to
		 ULONGEST instead.  */
	      if (PyErr_ExceptionMatches (PyExc_OverflowError))
		{
		  gdbpy_err_fetch fetched_error;
		  gdbpy_ref<> zero = gdb_py_object_from_longest (0);

		  /* Check whether obj is positive.  */
		  if (PyObject_RichCompareBool (obj, zero.get (), Py_GT) > 0)
		    {
		      ULONGEST ul;

		      ul = PyLong_AsUnsignedLongLong (obj);
		      if (! PyErr_Occurred ())
			value = value_from_ulongest (builtin_type_upylong, ul);
		    }
		  else
		    {
		      /* There's nothing we can do.  */
		      fetched_error.restore ();
		    }
		}
	    }
	  else
	    value = value_from_longest (builtin_type_pylong, l);
	}
#if PY_MAJOR_VERSION == 2
      else if (PyInt_Check (obj))
	{
	  long l = PyInt_AsLong (obj);

	  if (! PyErr_Occurred ())
	    value = value_from_longest (builtin_type_pyint, l);
	}
#endif
      else if (PyFloat_Check (obj))
	{
	  double d = PyFloat_AsDouble (obj);

	  if (! PyErr_Occurred ())
	    value = value_from_host_double (builtin_type_pyfloat, d);
	}
      else if (gdbpy_is_string (obj))
	{
	  gdb::unique_xmalloc_ptr<char> s
	    = python_string_to_target_string (obj);
	  if (s != NULL)
	    value = value_cstring (s.get (), strlen (s.get ()),
				   builtin_type_pychar);
	}
      else if (PyObject_TypeCheck (obj, &value_object_type))
	value = value_copy (((value_object *) obj)->value);
      else if (gdbpy_is_lazy_string (obj))
	{
	  PyObject *result;

	  result = PyObject_CallMethodObjArgs (obj, gdbpy_value_cst,  NULL);
	  value = value_copy (((value_object *) result)->value);
	}
      else
#ifdef IS_PY3K
	PyErr_Format (PyExc_TypeError,
		      _("Could not convert Python object: %S."), obj);
#else
	PyErr_Format (PyExc_TypeError,
		      _("Could not convert Python object: %s."),
		      PyString_AsString (PyObject_Str (obj)));
#endif
    }
  catch (const gdb_exception &except)
    {
      gdbpy_convert_exception (except);
      return NULL;
    }

  return value;
}

/* Returns value object in the ARGth position in GDB's history.  */
PyObject *
gdbpy_history (PyObject *self, PyObject *args)
{
  int i;
  struct value *res_val = NULL;	  /* Initialize to appease gcc warning.  */

  if (!PyArg_ParseTuple (args, "i", &i))
    return NULL;

  try
    {
      res_val = access_value_history (i);
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  return value_to_value_object (res_val);
}

/* Return the value of a convenience variable.  */
PyObject *
gdbpy_convenience_variable (PyObject *self, PyObject *args)
{
  const char *varname;
  struct value *res_val = NULL;

  if (!PyArg_ParseTuple (args, "s", &varname))
    return NULL;

  try
    {
      struct internalvar *var = lookup_only_internalvar (varname);

      if (var != NULL)
	{
	  res_val = value_of_internalvar (python_gdbarch, var);
	  if (value_type (res_val)->code () == TYPE_CODE_VOID)
	    res_val = NULL;
	}
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  if (res_val == NULL)
    Py_RETURN_NONE;

  return value_to_value_object (res_val);
}

/* Set the value of a convenience variable.  */
PyObject *
gdbpy_set_convenience_variable (PyObject *self, PyObject *args)
{
  const char *varname;
  PyObject *value_obj;
  struct value *value = NULL;

  if (!PyArg_ParseTuple (args, "sO", &varname, &value_obj))
    return NULL;

  /* None means to clear the variable.  */
  if (value_obj != Py_None)
    {
      value = convert_value_from_python (value_obj);
      if (value == NULL)
	return NULL;
    }

  try
    {
      if (value == NULL)
	{
	  struct internalvar *var = lookup_only_internalvar (varname);

	  if (var != NULL)
	    clear_internalvar (var);
	}
      else
	{
	  struct internalvar *var = lookup_internalvar (varname);

	  set_internalvar (var, value);
	}
    }
  catch (const gdb_exception &except)
    {
      GDB_PY_HANDLE_EXCEPTION (except);
    }

  Py_RETURN_NONE;
}

/* Returns 1 in OBJ is a gdb.Value object, 0 otherwise.  */

int
gdbpy_is_value_object (PyObject *obj)
{
  return PyObject_TypeCheck (obj, &value_object_type);
}

int
gdbpy_initialize_values (void)
{
  if (PyType_Ready (&value_object_type) < 0)
    return -1;

  return gdb_pymodule_addobject (gdb_module, "Value",
				 (PyObject *) &value_object_type);
}



static gdb_PyGetSetDef value_object_getset[] = {
  { "address", valpy_get_address, NULL, "The address of the value.",
    NULL },
  { "is_optimized_out", valpy_get_is_optimized_out, NULL,
    "Boolean telling whether the value is optimized "
    "out (i.e., not available).",
    NULL },
  { "type", valpy_get_type, NULL, "Type of the value.", NULL },
  { "dynamic_type", valpy_get_dynamic_type, NULL,
    "Dynamic type of the value.", NULL },
  { "is_lazy", valpy_get_is_lazy, NULL,
    "Boolean telling whether the value is lazy (not fetched yet\n\
from the inferior).  A lazy value is fetched when needed, or when\n\
the \"fetch_lazy()\" method is called.", NULL },
  {NULL}  /* Sentinel */
};

static PyMethodDef value_object_methods[] = {
  { "cast", valpy_cast, METH_VARARGS, "Cast the value to the supplied type." },
  { "dynamic_cast", valpy_dynamic_cast, METH_VARARGS,
    "dynamic_cast (gdb.Type) -> gdb.Value\n\
Cast the value to the supplied type, as if by the C++ dynamic_cast operator."
  },
  { "reinterpret_cast", valpy_reinterpret_cast, METH_VARARGS,
    "reinterpret_cast (gdb.Type) -> gdb.Value\n\
Cast the value to the supplied type, as if by the C++\n\
reinterpret_cast operator."
  },
  { "dereference", valpy_dereference, METH_NOARGS, "Dereferences the value." },
  { "referenced_value", valpy_referenced_value, METH_NOARGS,
    "Return the value referenced by a TYPE_CODE_REF or TYPE_CODE_PTR value." },
  { "reference_value", valpy_lvalue_reference_value, METH_NOARGS,
    "Return a value of type TYPE_CODE_REF referencing this value." },
  { "rvalue_reference_value", valpy_rvalue_reference_value, METH_NOARGS,
    "Return a value of type TYPE_CODE_RVALUE_REF referencing this value." },
  { "const_value", valpy_const_value, METH_NOARGS,
    "Return a 'const' qualied version of the same value." },
  { "lazy_string", (PyCFunction) valpy_lazy_string,
    METH_VARARGS | METH_KEYWORDS,
    "lazy_string ([encoding]  [, length]) -> lazy_string\n\
Return a lazy string representation of the value." },
  { "string", (PyCFunction) valpy_string, METH_VARARGS | METH_KEYWORDS,
    "string ([encoding] [, errors] [, length]) -> string\n\
Return Unicode string representation of the value." },
  { "fetch_lazy", valpy_fetch_lazy, METH_NOARGS,
    "Fetches the value from the inferior, if it was lazy." },
  { "format_string", (PyCFunction) valpy_format_string,
    METH_VARARGS | METH_KEYWORDS,
    "format_string (...) -> string\n\
Return a string representation of the value using the specified\n\
formatting options" },
  {NULL}  /* Sentinel */
};

static PyNumberMethods value_object_as_number = {
  valpy_add,
  valpy_subtract,
  valpy_multiply,
#ifndef IS_PY3K
  valpy_divide,
#endif
  valpy_remainder,
  NULL,			      /* nb_divmod */
  valpy_power,		      /* nb_power */
  valpy_negative,	      /* nb_negative */
  valpy_positive,	      /* nb_positive */
  valpy_absolute,	      /* nb_absolute */
  valpy_nonzero,	      /* nb_nonzero */
  valpy_invert,		      /* nb_invert */
  valpy_lsh,		      /* nb_lshift */
  valpy_rsh,		      /* nb_rshift */
  valpy_and,		      /* nb_and */
  valpy_xor,		      /* nb_xor */
  valpy_or,		      /* nb_or */
#ifdef IS_PY3K
  valpy_long,		      /* nb_int */
  NULL,			      /* reserved */
#else
  NULL,			      /* nb_coerce */
  valpy_int,		      /* nb_int */
  valpy_long,		      /* nb_long */
#endif
  valpy_float,		      /* nb_float */
#ifndef IS_PY3K
  NULL,			      /* nb_oct */
  NULL,                       /* nb_hex */
#endif
  NULL,                       /* nb_inplace_add */
  NULL,                       /* nb_inplace_subtract */
  NULL,                       /* nb_inplace_multiply */
#ifndef IS_PY3K
  NULL,                       /* nb_inplace_divide */
#endif
  NULL,                       /* nb_inplace_remainder */
  NULL,                       /* nb_inplace_power */
  NULL,                       /* nb_inplace_lshift */
  NULL,                       /* nb_inplace_rshift */
  NULL,                       /* nb_inplace_and */
  NULL,                       /* nb_inplace_xor */
  NULL,                       /* nb_inplace_or */
  NULL,                       /* nb_floor_divide */
  valpy_divide,               /* nb_true_divide */
  NULL,			      /* nb_inplace_floor_divide */
  NULL,			      /* nb_inplace_true_divide */
  valpy_long,		      /* nb_index */
};

static PyMappingMethods value_object_as_mapping = {
  valpy_length,
  valpy_getitem,
  valpy_setitem
};

PyTypeObject value_object_type = {
  PyVarObject_HEAD_INIT (NULL, 0)
  "gdb.Value",			  /*tp_name*/
  sizeof (value_object),	  /*tp_basicsize*/
  0,				  /*tp_itemsize*/
  valpy_dealloc,		  /*tp_dealloc*/
  0,				  /*tp_print*/
  0,				  /*tp_getattr*/
  0,				  /*tp_setattr*/
  0,				  /*tp_compare*/
  0,				  /*tp_repr*/
  &value_object_as_number,	  /*tp_as_number*/
  0,				  /*tp_as_sequence*/
  &value_object_as_mapping,	  /*tp_as_mapping*/
  valpy_hash,		          /*tp_hash*/
  valpy_call,	                  /*tp_call*/
  valpy_str,			  /*tp_str*/
  0,				  /*tp_getattro*/
  0,				  /*tp_setattro*/
  0,				  /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES
  | Py_TPFLAGS_BASETYPE,	  /*tp_flags*/
  "GDB value object",		  /* tp_doc */
  0,				  /* tp_traverse */
  0,				  /* tp_clear */
  valpy_richcompare,		  /* tp_richcompare */
  0,				  /* tp_weaklistoffset */
  0,				  /* tp_iter */
  0,				  /* tp_iternext */
  value_object_methods,		  /* tp_methods */
  0,				  /* tp_members */
  value_object_getset,		  /* tp_getset */
  0,				  /* tp_base */
  0,				  /* tp_dict */
  0,				  /* tp_descr_get */
  0,				  /* tp_descr_set */
  0,				  /* tp_dictoffset */
  0,				  /* tp_init */
  0,				  /* tp_alloc */
  valpy_new			  /* tp_new */
};
