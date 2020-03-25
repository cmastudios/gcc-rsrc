;; Machine description for RSRC

;; Copyright (C) 2020 Connor Monahan

;; Based heavily on file from Moxie port, written by Anthony Green.

;; Copyright (C) 2009-2014 Free Software Foundation, Inc.

;; This file is part of GCC.

;; GCC is free software; you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published
;; by the Free Software Foundation; either version 3, or (at your
;; option) any later version.

;; GCC is distributed in the hope that it will be useful, but WITHOUT
;; ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
;; or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
;; License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING3.  If not see
;; <http://www.gnu.org/licenses/>.

;; -------------------------------------------------------------------------
;; RSRC specific constraints, predicates and attributes
;; -------------------------------------------------------------------------

(include "constraints.md")
(include "predicates.md")

; Most instructions are two bytes long.
(define_attr "length" "" (const_int 6))

; We have to save the return address in a register. See RISC-V port for similar.
(define_constants
  [(RETURN_ADDR_REGNUM		15)
])

;; -------------------------------------------------------------------------
;; nop instruction
;; -------------------------------------------------------------------------

(define_insn "nop"
  [(const_int 0)]
  ""
  "nop")

;; -------------------------------------------------------------------------
;; Arithmetic instructions
;; -------------------------------------------------------------------------

(define_insn "addsi3"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r,r,r")
	  (plus:SI
	   (match_operand:SI 1 "register_operand" "0,0,0,r,r")
	   (match_operand:SI 2 "rsrc_add_operand" "I,N,r,r,J")))]
  ""
  "@
  addi	%0, %0, %2
  addi	%0, %0, %2
  add	%0, %0, %2
  add	%0, %1, %2
  addi	%0, %1, %2")

(define_insn "subsi3"
  [(set (match_operand:SI 0 "register_operand" "=r,r,r")
	  (minus:SI
	   (match_operand:SI 1 "register_operand" "0,0,r")
	   (match_operand:SI 2 "rsrc_sub_operand" "I,r,r")))]
  ""
  "@
  addi	%0, %0, -%2
  sub	%0, %0, %2
  sub	%0, %1, %2")


;; -------------------------------------------------------------------------
;; Unary arithmetic instructions
;; -------------------------------------------------------------------------

(define_insn "negsi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
	  (neg:SI (match_operand:SI 1 "register_operand" "r")))]
  ""
  "neg	%0, %1")

(define_insn "one_cmplsi2"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(not:SI (match_operand:SI 1 "register_operand" "r")))]
  ""
  "not	%0, %1")

;; -------------------------------------------------------------------------
;; Logical operators
;; -------------------------------------------------------------------------

(define_insn "andsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(and:SI (match_operand:SI 1 "register_operand" "0")
		(match_operand:SI 2 "register_operand" "r")))]
  ""
{
  return "and	%0, %0, %2";
})

(define_insn "iorsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(ior:SI (match_operand:SI 1 "register_operand" "0")
		(match_operand:SI 2 "register_operand" "r")))]
  ""
{
  return "or	%0, %0, %2";
})

;; -------------------------------------------------------------------------
;; Shifters
;; -------------------------------------------------------------------------

(define_insn "ashlsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(ashift:SI (match_operand:SI 1 "register_operand" "0")
		   (match_operand:SI 2 "register_operand" "r")))]
  ""
{
  return "shl	%0, %0, %2";
})

(define_insn "ashrsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(ashiftrt:SI (match_operand:SI 1 "register_operand" "0")
		     (match_operand:SI 2 "register_operand" "r")))]
  ""
{
  return "shra	%0, %0, %2";
})

(define_insn "lshrsi3"
  [(set (match_operand:SI 0 "register_operand" "=r")
	(lshiftrt:SI (match_operand:SI 1 "register_operand" "0")
		     (match_operand:SI 2 "register_operand" "r")))]
  ""
{
  return "shr	%0, %0, %2";
})

;; -------------------------------------------------------------------------
;; Move instructions
;; -------------------------------------------------------------------------

;; SImode

;; Push a register onto the stack
(define_insn "movsi_push"
  [(set (mem:SI (pre_dec:SI (reg:SI 31)))
  	(match_operand:SI 0 "register_operand" "r"))]
  ""
  "addi	r31, r31, -4\n\tst	%0, 0(r31)")

;; Pop a register from the stack
(define_insn "movsi_pop"
  [(set (match_operand:SI 1 "register_operand" "=r")
  	(mem:SI (post_inc:SI (match_operand:SI 0 "register_operand" "r"))))]
  ""
  "ld	%1, %0\n\taddi	%0, %0, 4")

(define_expand "movsi"
   [(set (match_operand:SI 0 "general_operand" "")
 	(match_operand:SI 1 "general_operand" ""))]
   ""
  "
{
  /* If this is a store, force the value into a register.  */
  if (! (reload_in_progress || reload_completed))
  {
    if (MEM_P (operands[0]))
    {
      operands[1] = force_reg (SImode, operands[1]);
      if (MEM_P (XEXP (operands[0], 0)))
        operands[0] = gen_rtx_MEM (SImode, force_reg (SImode, XEXP (operands[0], 0)));
    }
    else 
      if (MEM_P (operands[1])
          && MEM_P (XEXP (operands[1], 0)))
        operands[1] = gen_rtx_MEM (SImode, force_reg (SImode, XEXP (operands[1], 0)));
  }
}")

(define_insn "*movsi"
  [(set (match_operand:SI 0 "nonimmediate_operand" "=r,r,C,r")
	(match_operand:SI 1 "rsrc_general_movsrc_operand" "r,i,r,C"))]
  "register_operand (operands[0], SImode)
   || register_operand (operands[1], SImode)"
  "@
   or	%0, %1, %1
   la	%0, %1
   st	%1, %0
   ld	%0, %1")

; TODO: support storing chars

(define_expand "movqi"
    [(set (match_operand:QI 0 "general_operand" "")
 	(match_operand:QI 1 "general_operand" ""))]
   ""
  "
{
  /* If this is a store, force the value into a register.  */
    if (MEM_P (operands[0]))
    {
      operands[1] = force_reg (QImode, operands[1]);
    }
}")

(define_insn "*movqi"
  [(set (match_operand:QI 0 "nonimmediate_operand" "=r,r,C,r")
	(match_operand:QI 1 "rsrc_general_movsrc_operand" "r,i,r,C"))]
  "register_operand (operands[0], QImode)
   || register_operand (operands[1], QImode)"
  "@
   or	%0, %1, %1
   la	%0, %1
   st	%1, %0
   ld	%0, %1\n\tla	r0, %1\n\tandi	r0, r0, 3\n\tneg r0, r0\n\taddi r0, r0, 3\n\tshl	r0, r0, 3\n\tshr	%0, %0, r0\n\tandi	%0, %0, 255"
  [(set_attr "length"	"4,4,4,32")])

; TODO: support storing shorts

(define_expand "movhi"
    [(set (match_operand:HI 0 "general_operand" "")
 	(match_operand:HI 1 "general_operand" ""))]
   ""
  "
{
  /* If this is a store, force the value into a register.  */
    if (MEM_P (operands[0]))
    {
      operands[1] = force_reg (HImode, operands[1]);
    }
}")

(define_insn "*movhi"
  [(set (match_operand:HI 0 "nonimmediate_operand" "=r,r,C,r")
	(match_operand:HI 1 "rsrc_general_movsrc_operand" "r,i,r,C"))]
  "register_operand (operands[0], HImode)
   || register_operand (operands[1], HImode)"
  "@
   or	%0, %1, %1
   la	%0, %1
   st	%1, %0
   ld	%0, %1\n\tla	r0, %1\n\tandi	r0, r0, 1\n\tshl	r0, r0, 4\n\tshr	%0, %0, r0\n\tandi	%0, %0, 65535"
  [(set_attr "length"	"4,4,4,24")])


;; -------------------------------------------------------------------------
;; Compare instructions
;; -------------------------------------------------------------------------

(define_expand "cbranchsi4"
	[(set (match_dup 4)
	  (compare:SI
	   (match_operand:SI 1 "general_operand" "")
	   (match_operand:SI 2 "general_operand" "")))
	   (parallel [
	(set (pc)
        (if_then_else (match_operator 0 "comparison_operator"
                       [ (match_dup 4)(const_int 0)])
                     (label_ref (match_operand 3 "" ""))
                      (pc)))
				(clobber (scratch:SI))
					  ])
					  ]
  ""
  "
  /* Force the compare operands into registers.  */
  if (GET_CODE (operands[1]) != REG)
	operands[1] = force_reg (SImode, operands[1]);
  if (GET_CODE (operands[2]) != REG)
	operands[2] = force_reg (SImode, operands[2]);
  operands[4] = gen_reg_rtx(SImode);
  ")

; Why not just use the minus operation above? GCC gets confused building libgcc.

(define_insn "*cmpsi"
  [(set (match_operand:SI 0 "register_operand" "=r")
	  (compare:SI
	   (match_operand:SI 1 "register_operand" "r")
	   (match_operand:SI 2 "rsrc_sub_operand" "r")))]
  ""
  "sub	%0, %1, %2")


;; -------------------------------------------------------------------------
;; Branch instructions
;; -------------------------------------------------------------------------

; TODO make these actually unsigned

; What is condEZ you might ask? Well, GCC requires all 10 types of comparison,
; while our CPU only has instructions for 4. To support this, we split the
; comparisons into condEZ (which only requires one instruction) and condOP
; (which requires a negation beforehand)

(define_code_iterator condez [ne eq lt ge ltu geu])
(define_code_attr CCez [(ne "nz") (eq "zr") (lt "mi")(ge "pl") (ltu "mi") (geu "pl")])

; TODO don't actually require a scratch register for condez

(define_insn "*b<condez:code>"
  [(set (pc)
	(if_then_else (condez (match_operand 0 "register_operand" "r")(const_int 0))
				(label_ref (match_operand 1 "" ""))
		      (pc)))
			  (clobber (match_scratch:SI 2 "=&r"))
			  ]
  ""
  "la	r0, %l1\n\tbr<CCez>	r0, %0"
  [(set_attr "length" "8")])

(define_code_iterator condop [le gt leu gtu])
(define_code_attr CCop [(le "pl") (gt "mi") (leu "pl") (gtu "mi")])

(define_insn "*b<condop:code>"
  [(set (pc)
	(if_then_else (condop (match_operand 0 "" "")(const_int 0))
				(label_ref (match_operand 1 "" ""))
		      (pc)))
			  (clobber (match_scratch:SI 2 "=&r"))
			  ]
  ""
  "la	r0, %l1\n\tneg %2, %0\n\tbr<CCop>	r0, %2"
  [(set_attr "length" "12")])

;; -------------------------------------------------------------------------
;; Call and Jump instructions
;; -------------------------------------------------------------------------

; replace_equiv_address is used to take a function name and store its memory
; location into a register.

(define_expand "call"
  [(parallel [(call (match_operand 0 "" "") (match_operand 1 "" ""))
				(use (match_operand 2 "" ""))
				(clobber (reg:SI RETURN_ADDR_REGNUM))])]
  ""
  "
	operands[0] = replace_equiv_address(operands[0], force_reg(Pmode, XEXP(operands[0], 0)));
  ")

(define_insn "*call"
  [(call (mem:QI (match_operand 0 "register_operand" "r")) (match_operand 1 "" ""))
				(use (match_operand 2 "" ""))
	 (clobber (reg:SI RETURN_ADDR_REGNUM))]
  ""
  "brl	r15, %0")

(define_expand "call_value"
  [(parallel [(set (match_operand 0 "" "") (call (match_operand 1 "" "") (match_operand 2 "" "")))
				(use (match_operand 3 "" ""))
				(clobber (reg:SI RETURN_ADDR_REGNUM))])]
  ""
  "
	operands[1] = replace_equiv_address(operands[1], force_reg(Pmode, XEXP(operands[1], 0)));
  ")

(define_insn "*call_value_indirect"
  [(set (match_operand 0 "register_operand" "=r")
	(call (mem:QI (match_operand
		       1 "register_operand" "r"))
	      (match_operand 2 "" "")))
				(use (match_operand 3 "" ""))
	 (clobber (reg:SI RETURN_ADDR_REGNUM))]
  ""
  "brl	r15, %1")

(define_insn "indirect_jump"
  [(set (pc) (match_operand:SI 0 "nonimmediate_operand" "r"))]
  ""
  "br	%0")

; For some reason, replace_equiv_address doesn't work when the argument is a
; label instead of a symbol. To fix this, we just load it into our scratch
; register. Same technique is used for conditional branching.

(define_insn "jump"
  [(set (pc)
	(label_ref (match_operand 0 "" "")))
	]
  ""
  "la	r0, %l0\n\tbr	r0"
  [(set_attr "length" "8")])


;; -------------------------------------------------------------------------
;; Prologue & Epilogue
;; -------------------------------------------------------------------------

; save r15
(define_expand "prologue"
  [(clobber (const_int 0))]
  ""
  "
{
  rsrc_expand_prologue ();
  DONE;
}
")

; restore r15
(define_expand "epilogue"
  [(return)]
  ""
  "
{
  rsrc_expand_epilogue ();
  DONE;
}
")

; at this point, we popped the old frame pointer and the old return address.
(define_insn "returner"
  [(return)]
  "reload_completed"
  "br	r15")
