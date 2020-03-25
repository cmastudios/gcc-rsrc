;; Constraint definitions for RSRC

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
;; Constraints
;; -------------------------------------------------------------------------

(define_constraint "A"
  "An absolute address."
  (and (match_code "mem")
       (ior (match_test "GET_CODE (XEXP (op, 0)) == SYMBOL_REF")
	    (match_test "GET_CODE (XEXP (op, 0)) == LABEL_REF")
	    (match_test "GET_CODE (XEXP (op, 0)) == CONST"))))

(define_constraint "B"
  "An offset address."
  (and (match_code "mem")
       (match_test "GET_CODE (XEXP (op, 0)) == PLUS")))

(define_constraint "W"
  "A register indirect memory operand."
  (and (match_code "mem")
       (match_test "REG_P (XEXP (op, 0))
		    && REGNO_OK_FOR_BASE_P (REGNO (XEXP (op, 0)))")))

(define_constraint "C"
	"Everything"
  (and (match_code "mem")
       (ior (match_test "GET_CODE (XEXP (op, 0)) == SYMBOL_REF")
	    (match_test "GET_CODE (XEXP (op, 0)) == LABEL_REF")
	    (match_test "GET_CODE (XEXP (op, 0)) == CONST")
       (match_test "GET_CODE (XEXP (op, 0)) == PLUS")
       (match_test "REG_P (XEXP (op, 0))
		    && REGNO_OK_FOR_BASE_P (REGNO (XEXP (op, 0)))"))))

(define_constraint "O"
  "The constant zero"
  (and (match_code "const_int")
       (match_test "ival == 0")))

(define_constraint "I"
  "An 8-bit constant (0..65535)"
  (and (match_code "const_int")
       (match_test "ival >= 0 && ival <= 65535")))

(define_constraint "J"
  "An 8-bit constant (0..65535)"
  (and (match_code "const_int")
       (match_test "ival >= -65536 && ival <= 65535")))

(define_constraint "N"
  "A constant -(0..65536)"
  (and (match_code "const_int")
       (match_test "ival >= -65536 && ival <= 0")))
