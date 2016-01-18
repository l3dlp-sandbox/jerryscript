/* Copyright 2015 Samsung Electronics Co., Ltd.
 * Copyright 2015 University of Szeged.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecma-builtins.h"
#include "ecma-conversion.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-globals.h"
#include "ecma-helpers.h"
#include "ecma-lex-env.h"
#include "ecma-objects.h"
#include "ecma-try-catch-macro.h"
#include "opcodes.h"
#include "opcodes-variable-helpers.h"
#include "vm-defines.h"

/**
 * 'Function declaration' opcode handler.
 *
 * @return completion value
 *         returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_func_decl_n (vm_frame_ctx_t *frame_ctx_p, /**< interpreter context */
                    ecma_string_t *func_name_str_p, /**< function name */
                    const ecma_length_t args_num) /**< number of arguments */
{
  ecma_collection_header_t *formal_params_collection_p;

  if (args_num != 0)
  {
    formal_params_collection_p = ecma_new_strings_collection (NULL, 0);

    for (int i = 0; i < args_num; i++)
    {
      ecma_value_t arg_name_value = 0; // FIXME

      ecma_append_to_values_collection (formal_params_collection_p, arg_name_value, false);
    }
  }
  else
  {
    formal_params_collection_p = NULL;
  }

  const bool is_configurable_bindings = frame_ctx_p->is_eval_code;

  ecma_completion_value_t ret_value = ecma_op_function_declaration (frame_ctx_p->lex_env_p,
                                                                    func_name_str_p,
                                                                    frame_ctx_p->bytecode_header_p,
                                                                    formal_params_collection_p,
                                                                    frame_ctx_p->is_strict,
                                                                    is_configurable_bindings);

  return ret_value;
} /* opfunc_func_decl_n */

/**
 * 'Function call' opcode handler.
 *
 * See also: ECMA-262 v5, 11.2.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_call_n (vm_frame_ctx_t *frame_ctx_p, /**< interpreter context */
               ecma_value_t this_value, /**< this object value */
               ecma_value_t func_value, /**< function object value */
               uint8_t args_num, /**< number of arguments */
               ecma_value_t *stack_p) /**< stack pointer */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  if (this_value == 0)
  {
    ecma_completion_value_t this_comp_value;
    this_comp_value = ecma_op_implicit_this_value (frame_ctx_p->ref_base_lex_env_p);

    if (ecma_is_completion_value_throw (this_comp_value))
    {
      return this_comp_value;
    }

    this_value = ecma_get_completion_value_value (this_comp_value);
  }

  JERRY_ASSERT (!frame_ctx_p->is_call_in_direct_eval_form);

  // FIXME: Fix fucntion calls in eval!
  uint8_t call_flags;
  ecma_collection_header_t *arg_collection_p = ecma_new_values_collection (NULL, 0, true);

  for (int i = 0; i < args_num; i++)
  {
    ecma_append_to_values_collection (arg_collection_p, stack_p[i], true);
  }

  if (!ecma_op_is_callable (func_value))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    if (call_flags & OPCODE_CALL_FLAGS_DIRECT_CALL_TO_EVAL_FORM)
    {
      frame_ctx_p->is_call_in_direct_eval_form = true;
    }

    ecma_object_t *func_obj_p = ecma_get_object_from_value (func_value);

    ECMA_TRY_CATCH (call_ret_value,
                    ecma_op_function_call (func_obj_p,
                                           this_value,
                                           arg_collection_p),
                    ret_value);

    ret_value = ecma_make_normal_completion_value (ecma_copy_value (call_ret_value, true));

    ECMA_FINALIZE (call_ret_value);

    if (call_flags & OPCODE_CALL_FLAGS_DIRECT_CALL_TO_EVAL_FORM)
    {
      JERRY_ASSERT (frame_ctx_p->is_call_in_direct_eval_form);
      frame_ctx_p->is_call_in_direct_eval_form = false;
    }
    else
    {
      JERRY_ASSERT (!frame_ctx_p->is_call_in_direct_eval_form);
    }
  }

  ecma_free_values_collection (arg_collection_p, true);
  ecma_free_value (this_value, true);

  return ret_value;
} /* opfunc_call_n */

/**
 * 'Constructor call' opcode handler.
 *
 * See also: ECMA-262 v5, 11.2.2
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value.
 */
ecma_completion_value_t
opfunc_construct_n (vm_frame_ctx_t *frame_ctx_p, /**< interpreter context */
                    ecma_value_t constructor_value, /**< constructor object value */
                    uint8_t args_num, /**< number of arguments */
                    ecma_value_t *stack_p) /**< stack pointer */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  JERRY_ASSERT (!frame_ctx_p->is_call_in_direct_eval_form);

  ecma_collection_header_t *arg_collection_p = ecma_new_values_collection (NULL, 0, true);

  for (int i = 0; i < args_num; i++)
  {
    ecma_append_to_values_collection (arg_collection_p, stack_p[i], true);
  }

  if (!ecma_is_constructor (constructor_value))
  {
    ret_value = ecma_make_throw_obj_completion_value (ecma_new_standard_error (ECMA_ERROR_TYPE));
  }
  else
  {
    ecma_object_t *constructor_obj_p = ecma_get_object_from_value (constructor_value);

    ECMA_TRY_CATCH (construction_ret_value,
                    ecma_op_function_construct (constructor_obj_p,
                                                arg_collection_p),
                    ret_value);

    ret_value = ecma_make_normal_completion_value (ecma_copy_value (construction_ret_value, true));

    ECMA_FINALIZE (construction_ret_value);
  }

  ecma_free_values_collection (arg_collection_p, true);

  return ret_value;
} /* opfunc_construct_n */

/**
 * 'Variable declaration' opcode handler.
 *
 * See also: ECMA-262 v5, 10.5 - Declaration binding instantiation (block 8).
 *
 * @return completion value
 *         Returned value is simple and so need not be freed.
 *         However, ecma_free_completion_value may be called for it, but it is a no-op.
 */
ecma_completion_value_t
vm_var_decl (vm_frame_ctx_t *frame_ctx_p, /**< interpreter context */
             ecma_string_t *var_name_str_p) /**< variable name */
{
  if (!ecma_op_has_binding (frame_ctx_p->lex_env_p, var_name_str_p))
  {
    const bool is_configurable_bindings = frame_ctx_p->is_eval_code;

    ecma_completion_value_t completion = ecma_op_create_mutable_binding (frame_ctx_p->lex_env_p,
                                                                         var_name_str_p,
                                                                         is_configurable_bindings);

    JERRY_ASSERT (ecma_is_completion_value_empty (completion));

    /* Skipping SetMutableBinding as we have already checked that there were not
     * any binding with specified name in current lexical environment
     * and CreateMutableBinding sets the created binding's value to undefined */
    JERRY_ASSERT (ecma_is_completion_value_normal_simple_value (ecma_op_get_binding_value (frame_ctx_p->lex_env_p,
                                                                                           var_name_str_p,
                                                                                           true),
                                                                ECMA_SIMPLE_VALUE_UNDEFINED));
  }
  return ecma_make_empty_completion_value ();
} /* vm_var_decl */

/**
 * 'Logical NOT Operator' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.9
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_logical_not (ecma_value_t left_value) /**< left value */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ecma_simple_value_t old_value = ECMA_SIMPLE_VALUE_TRUE;
  ecma_completion_value_t to_bool_value = ecma_op_to_boolean (left_value);

  if (ecma_is_value_true (ecma_get_completion_value_value (to_bool_value)))
  {
    old_value = ECMA_SIMPLE_VALUE_FALSE;
  }

  ret_value = ecma_make_simple_completion_value (old_value);

  return ret_value;
} /* opfunc_logical_not */

/**
 * 'typeof' opcode handler.
 *
 * See also: ECMA-262 v5, 11.4.3
 *
 * @return completion value
 *         Returned value must be freed with ecma_free_completion_value
 */
ecma_completion_value_t
opfunc_typeof (ecma_value_t left_value) /**< left value */
{
  ecma_completion_value_t ret_value = ecma_make_empty_completion_value ();

  ecma_string_t *type_str_p = NULL;

  if (ecma_is_value_undefined (left_value))
  {
    type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_UNDEFINED);
  }
  else if (ecma_is_value_null (left_value))
  {
    type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_OBJECT);
  }
  else if (ecma_is_value_boolean (left_value))
  {
    type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_BOOLEAN);
  }
  else if (ecma_is_value_number (left_value))
  {
    type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_NUMBER);
  }
  else if (ecma_is_value_string (left_value))
  {
    type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_STRING);
  }
  else
  {
    JERRY_ASSERT (ecma_is_value_object (left_value));

    if (ecma_op_is_callable (left_value))
    {
      type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_FUNCTION);
    }
    else
    {
      type_str_p = ecma_get_magic_string (LIT_MAGIC_STRING_OBJECT);
    }
  }

  ret_value = ecma_make_normal_completion_value (ecma_make_string_value (type_str_p));

  return ret_value;
} /* opfunc_typeof */

/**
 * Update getter or setter for object literals.
 */
void
opfunc_set_accessor (bool is_getter,
                     ecma_value_t object,
                     ecma_value_t accessor_name,
                     ecma_value_t accessor)
{
  ecma_object_t *object_p = ecma_get_object_from_value (object);
  ecma_string_t *accessor_name_p = ecma_get_string_from_value (accessor_name);
  ecma_property_t *property_p = ecma_find_named_property (object_p, accessor_name_p);

  if (property_p != NULL && property_p->type != ECMA_PROPERTY_NAMEDACCESSOR)
  {
    ecma_delete_property (object_p, property_p);
    property_p = NULL;
  }

  if (property_p == NULL)
  {
    ecma_object_t *getter_func_p = NULL;
    ecma_object_t *setter_func_p = NULL;

    if (is_getter)
    {
      getter_func_p = ecma_get_object_from_value (accessor);
    }
    else
    {
      setter_func_p = ecma_get_object_from_value (accessor);
    }

    ecma_create_named_accessor_property (object_p,
                                         accessor_name_p,
                                         getter_func_p,
                                         setter_func_p,
                                         true,
                                         true);
  }
  else if (is_getter)
  {
    ecma_object_t *getter_func_p = ecma_get_object_from_value (accessor);

    ecma_set_named_accessor_property_getter (object_p,
                                             property_p,
                                             getter_func_p);
  }
  else
  {
    ecma_object_t *setter_func_p = ecma_get_object_from_value (accessor);

    ecma_set_named_accessor_property_setter (object_p,
                                             property_p,
                                             setter_func_p);
  }
} /* opfunc_define_accessor */

/**
 * Deletes an object property.
 *
 * @return completion value
 */
ecma_completion_value_t
vm_op_delete_prop (ecma_value_t object, /**< base object */
                   ecma_value_t property, /**< property name */
                   bool is_strict) /**< strict mode */
{
  ecma_completion_value_t completion_value = ecma_make_empty_completion_value ();

  if (ecma_is_value_undefined (object))
  {
    JERRY_ASSERT (!is_strict);
    completion_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }
  else
  {
    completion_value = ecma_make_empty_completion_value ();

    ECMA_TRY_CATCH (check_coercible_ret,
                    ecma_op_check_object_coercible (object),
                    completion_value);
    ECMA_TRY_CATCH (str_name_value,
                    ecma_op_to_string (property),
                    completion_value);

    JERRY_ASSERT (ecma_is_value_string (str_name_value));
    ecma_string_t *name_string_p = ecma_get_string_from_value (str_name_value);

    ECMA_TRY_CATCH (obj_value, ecma_op_to_object (object), completion_value);

    JERRY_ASSERT (ecma_is_value_object (obj_value));
    ecma_object_t *obj_p = ecma_get_object_from_value (obj_value);
    JERRY_ASSERT (!ecma_is_lexical_environment (obj_p));

    ECMA_TRY_CATCH (delete_op_ret_val,
                    ecma_op_object_delete (obj_p, name_string_p, is_strict),
                    completion_value);

    completion_value = ecma_make_normal_completion_value (delete_op_ret_val);

    ECMA_FINALIZE (delete_op_ret_val);
    ECMA_FINALIZE (obj_value);
    ECMA_FINALIZE (str_name_value);
    ECMA_FINALIZE (check_coercible_ret);
  }

  return completion_value;
} /* vm_op_delete_prop */

/**
 * Deletes a variable.
 *
 * @return completion value
 */
ecma_completion_value_t
vm_op_delete_var (lit_cpointer_t name_literal, /**< name literal */
                  ecma_object_t *lex_env_p, /**< lexical environment */
                  bool is_strict) /**< strict mode */
{
  ecma_completion_value_t completion_value = ecma_make_empty_completion_value ();

  ecma_string_t *var_name_str_p;

  var_name_str_p = ecma_new_ecma_string_from_lit_cp (name_literal);
  ecma_reference_t ref = ecma_op_get_identifier_reference (lex_env_p,
                                                           var_name_str_p,
                                                           is_strict);

  JERRY_ASSERT (!ref.is_strict);

  if (ecma_is_value_undefined (ref.base))
  {
    completion_value = ecma_make_simple_completion_value (ECMA_SIMPLE_VALUE_TRUE);
  }
  else
  {
    ecma_object_t *ref_base_lex_env_p = ecma_op_resolve_reference_base (lex_env_p, var_name_str_p);

    JERRY_ASSERT (ecma_is_lexical_environment (ref_base_lex_env_p));

    ECMA_TRY_CATCH (delete_op_ret_val,
                    ecma_op_delete_binding (ref_base_lex_env_p,
                                            ECMA_GET_NON_NULL_POINTER (ecma_string_t,
                                                                       ref.referenced_name_cp)),
                    completion_value);

    completion_value = ecma_make_normal_completion_value (delete_op_ret_val);

    ECMA_FINALIZE (delete_op_ret_val);

  }

  ecma_free_reference (ref);
  ecma_deref_ecma_string (var_name_str_p);

  return completion_value;
} /* vm_op_delete_var */
