#pragma once

#include "canopy/types.hpp"

#include "syntax/seed_interpreter.hpp"

namespace silva::cedar {

  // Translation of https://www.quut.com/c/ANSI-C-grammar-y.html.
  //
  const string_view_t seed_str = R"'(
tokenizer Cedar:
  operator = [ ';' '*' '~' '++' '+=' '+' '--' '-=' '-' '!=' '!' '...' ]
  include tokenizer FreeForm

language Cedar:
  ⊙ = Decl *
  Decl:
    ⊙ = Decl.Specifiers Type.Specifier InitDeclarator ( ';' | Stmt.Compound ) \
      | StaticAssert
    Specifiers = ( StorageClassSpecifier | FunctionSpecifier | AlignmentSpecifier | Type.Qualifier ) *
    StorageClassSpecifier = ( 'typedef' | 'extern' | 'static' | '_Thread_local' | 'auto' | 'register' )
    FunctionSpecifier = 'inline' | '_Noreturn'
    AlignmentSpecifier = ( '_Alignas' | 'alignas' ) '(' ( TypeName | Expr.Const ) ')'
    StaticAssert = '_Static_assert' '(' Expr.Const ',' string ')' ';'

  Type:
    Specifier = ( 'void' | 'char' | 'short' | 'int' | 'long'
                | 'float' | 'double' | 'signed' | 'unsigned'
                | '_Bool' | '_Complex' | '_Imaginary'
                | AtomicTypeSpecifier
                | StructOrUnionSpecifier
                | EnumSpecifier
                | TypedefName
                )
    Qualifier = ( 'const' | 'volatile' | 'restrict' | '_Atomic' )

  EnumSpecifier = 'enum' identifier ? '{' Enumerator ( ',' Enumerator ) * ',' ? '}'
  Enumerator = EnumerationConstant ( '=' Expr.Const ) ?
  TypeName = SpecifierQualifierList AbstractDeclarator ?

  TypedefName = identifier # TODO: should only match identifiers that have previously been typedef'ed.
  EnumerationConstant = identifier

  AtomicTypeSpecifier = '_Atomic' '(' TypeName ')'
  StructOrUnionSpecifier = ( 'struct' | 'union' ) identifier ? ( '{' StructDeclaration * '}' ) ?
  StructDeclaration = SpecifierQualifierList StructDeclaratorList ? | Decl.StaticAssert
  SpecifierQualifierList = ( Type.Specifier | Type.Qualifier ) *
  StructDeclaratorList = StructDeclarator ( ',' StructDeclarator ) *
  StructDeclarator = Declarator ( ':' Expr.Const ) ? | ':' Expr.Const
  InitDeclarator = Declarator ( '=' Initializer ) ?
  Declarator = Pointer ? DirectDeclarator
  AbstractDeclarator = Pointer DirectAbstractDeclarator ? | DirectAbstractDeclarator
  DirectAbstractDeclarator = ( '(' AbstractDeclarator ')' | DirectAbstractDeclaratorExt ) DirectAbstractDeclaratorExt *
  DirectAbstractDeclaratorExt = CommonDeclaratorExt
  Pointer = '*' Type.Qualifier * Pointer ?
  DirectDeclarator = ( identifier | '(' Declarator ')' ) DirectDeclaratorExt *
  DirectDeclaratorExt = ( CommonDeclaratorExt
                        | '(' IdentifierList ')'
                        | '[' Type.Qualifier * '*' ']'
                        )
  CommonDeclaratorExt = alias ( '[' ']'
                              | '[' '*' ']'
                              | '[' 'static' Type.Qualifier * Expr ']'
                              | '[' 'static' Expr ']'
                              | '[' Type.Qualifier * 'static' Expr ']'
                              | '[' Type.Qualifier * Expr ']'
                              | '[' Type.Qualifier * ']'
                              | '[' Expr ']'
                              | '(' ParameterList ')'
                              | '(' ')'
                              )
  ParameterList = ParameterDeclaration ( ',' ParameterDeclaration ) *
  ParameterDeclaration = Decl.Specifiers Type.Specifier TypeName ( Declarator | AbstractDeclarator ) ? | '...'
  IdentifierList = identifier ( ',' identifier ) *

  Initializer = '{' InitializerList ',' ? '}' | Expr
  InitializerList = Designation ? Initializer ( ',' Designation ? Initializer ) *
  Designation = Designator + '='
  Designator = '.' identifier | '[' Expr.Const ']'

  Stmt:
    ⊙ = ( If | While | For | DoWhile | Switch | Case | Default | Goto | Continue | Break | Return | Label | Compound | ExprStmt )
    If = 'if' '(' Expr ')' Stmt ( 'else' Stmt ) ?
    While = 'while' '(' Expr ')' Stmt
    DoWhile = 'do' Stmt 'while' '(' Expr ')' ';'
    For = 'for' '(' ForInit Expr ? ';' Expr ? ')' Stmt
    ForInit = Decl | ExprStmt
    Switch = 'switch' '(' Expr ')' Stmt
    Case = 'case' Expr.Const ':' Stmt
    Default = 'default' ':' Stmt
    Goto = 'goto' identifier ';'
    Continue = 'continue' ';'
    Break = 'break' ';'
    Return = 'return' Expr ? ';'
    Label = identifier ':' Stmt
    Compound = '{' ( Stmt | Decl ) * '}'
    ExprStmt = Expr ? ';'

  Expr:
    ⊙ = axe Atom
      Parens  = nest atom_nest '(' ')'
      Postfix = ltr  postfix '++' '--' \
                     postfix_nest -> ExprOrNone '(' ')' '[' ']' \
                     infix '.' '->'
      Prefix  = rtl  prefix '++' '--' '+' '-' '!' '~' '*' '&' 'sizeof' '_Alignof'
      Mult    = ltr  infix '*' '/' '%'
      Add     = ltr  infix '+' '-'
      Shift   = ltr  infix '<<' '>>'
      Rel     = ltr  infix '<' '>' '<=' '>='
      Eq      = ltr  infix '==' '!='
      BitAnd  = ltr  infix '&'
      BitXor  = ltr  infix '^'
      BitOr   = ltr  infix '|'
      LogAnd  = ltr  infix '&&'
      LogOr   = ltr  infix '||'
      Tern    = rtl  ternary '?' ':'
      Assign  = rtl  infix '=' '+=' '-=' '*=' '/=' '%=' '<<=' '>>=' '&=' '^=' '|='
      Comma   = ltr  infix_flat ','
    Atom       = number | string + | identifier
    ExprOrNone = Expr | None
    Const = Expr # not allowed to be Expr.Comma or Expr.Assign
)'";

  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t);
}
