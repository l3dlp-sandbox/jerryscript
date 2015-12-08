/* Copyright 2014-2015 Samsung Electronics Co., Ltd.
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

#ifndef VM_H
#define VM_H

#include "ecma-globals.h"
#include "jrt.h"
#include "vm-defines.h"

#define VM_OC_LEFT_OPERAND_SHIFT 28
#define VM_OC_LEFT_OPERAND_MASK 0xf
#define VM_OC_LEFT_OPERAND_VALUE(V) (((V) & VM_OC_LEFT_OPERAND_MASK) << VM_OC_LEFT_OPERAND_SHIFT)
#define VM_OC_LEFT_OPERAND(O) (((O) >> VM_OC_LEFT_OPERAND_SHIFT) & VM_OC_LEFT_OPERAND_MASK)

#define VM_OC_RIGHT_OPERAND_SHIFT 24
#define VM_OC_RIGHT_OPERAND_MASK 0xf
#define VM_OC_RIGHT_OPERAND_VALUE(V) (((V) & VM_OC_RIGHT_OPERAND_MASK) << VM_OC_RIGHT_OPERAND_SHIFT)
#define VM_OC_RIGHT_OPERAND(O) (((O) >> VM_OC_RIGHT_OPERAND_SHIFT) & VM_OC_RIGHT_OPERAND_MASK)

enum {
  VM_OC_OP_NONE,
  VM_OC_OP_BRANCH_1,
  VM_OC_OP_BRANCH_2,
  VM_OC_OP_BRANCH_3,
  VM_OC_OP_BYTE,
  VM_OC_OP_LITERAL,
  VM_OC_OP_STACK
};

#define VM_OC_GROUP_SHIFT 16
#define VM_OC_GROUP_MASK 0xff
#define VM_OC_GROUP_VALUE(V) (((V) & 0xff) << VM_OC_GROUP_SHIFT)
#define VM_OC_GROUP(O) (((O) >> VM_OC_GROUP_SHIFT) & 0xff)

enum {
  VM_OC_GROUP_NONE,
  VM_OC_GROUP_POP,
  VM_OC_GROUP_PUSH,
  VM_OC_GROUP_PUSH_TWO,
  VM_OC_GROUP_RET,
  VM_OC_GROUP_CALL,

  VM_OC_GROUP_EQUAL,
  VM_OC_GROUP_NOT_EQUAL,
  VM_OC_GROUP_STRICT_EQUAL,
  VM_OC_GROUP_STRICT_NOT_EQUAL,

  VM_OC_GROUP_ADD,
  VM_OC_GROUP_SUB,
  VM_OC_GROUP_MUL,
  VM_OC_GROUP_DIV,
  VM_OC_GROUP_MOD
};

#define VM_OC_POST_PROCESS_SHIFT 12
#define VM_OC_POST_PROCESS_MASK 0xf
#define VM_OC_POST_PROCESS_VALUE(V) (((V) & VM_OC_POST_PROCESS_MASK) << VM_OC_POST_PROCESS_SHIFT)
#define VM_OC_POST_PROCESS(O) (((O) >> VM_OC_POST_PROCESS_SHIFT) & VM_OC_POST_PROCESS_MASK)

enum {
  VM_OC_POST_NONE,
  VM_OC_POST_PUSH_RESULT
};

extern void vm_init (const cbc_compiled_code_t *, bool);
extern void vm_finalize (void);
extern jerry_completion_code_t vm_run_global (void);
extern ecma_completion_value_t vm_run_eval (const cbc_compiled_code_t *, bool);

extern ecma_completion_value_t vm_loop (vm_frame_ctx_t *);
extern ecma_completion_value_t vm_run_from_pos (const cbc_compiled_code_t *,
                                                vm_instr_counter_t,
                                                ecma_value_t,
                                                ecma_object_t *,
                                                bool,
                                                bool);

extern opcode_scope_code_flags_t vm_get_scope_flags (const cbc_compiled_code_t *);

extern bool vm_is_strict_mode (void);
extern bool vm_is_direct_eval_form_call (void);

extern ecma_value_t vm_get_this_binding (void);
extern ecma_object_t *vm_get_lex_env (void);

#endif /* VM_H */
