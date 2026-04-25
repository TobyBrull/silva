#pragma once

#include "canopy/types.hpp"

#include "syntax/seed_interpreter.hpp"

namespace silva::cedar {

  // Translation of https://www.quut.com/c/ANSI-C-grammar-y.html.
  //
  const string_view_t seed_str = R"'(
tokenizer Cedar:
  operator = [ '*' '++' '+=' '--' '-=' '...' ]
  include tokenizer FreeForm

language Cedar:
  ⊙ = ( FunctionDefinition | Decl | StaticAssertDeclaration ) *

  FunctionDefinition = DeclarationSpecifiers Declarator Decl * Stmt.Compound
  Decl = DeclarationSpecifiers InitDeclaratorList ? ';' | StaticAssertDeclaration
  StaticAssertDeclaration = '_Static_assert' '(' Expr.Const ',' string ')' ';'

  DeclarationSpecifiers = ( StorageClassSpecifier | TypeSpecifier | TypeQualifier | FunctionSpecifier | AlignmentSpecifier ) *
  AlignmentSpecifier = ( '_Alignas' | 'alignas' ) '(' ( TypeName | Expr.Const ) ')'
  FunctionSpecifier = 'inline' | '_Noreturn'
  StorageClassSpecifier = ( 'typedef' | 'extern' | 'static' | '_Thread_local' | 'auto' | 'register' )

  TypeSpecifier = ( 'void' | 'char' | 'short' | 'int' | 'long'
                  | 'float' | 'double' | 'signed' | 'unsigned'
                  | '_Bool' | '_Complex' | '_Imaginary'
                  | AtomicTypeSpecifier
                  | StructOrUnionSpecifier
                  | EnumSpecifier
                  | TypedefName
                  )
  EnumSpecifier = 'enum' identifier ? '{' Enumerator ( ',' Enumerator ) * ',' ? '}'
  Enumerator = EnumerationConstant ( '=' Expr.Const ) ?
  TypeName = SpecifierQualifierList AbstractDeclarator

  TypedefName = identifier # TODO: should only match identifiers that have previously been typedef'ed.
  EnumerationConstant = identifier

  AtomicTypeSpecifier = '_Atomic' '(' TypeName ')'
  StructOrUnionSpecifier = ( 'struct' | 'union' ) identifier ? ( '{' StructDeclaration * '}' ) ?
  StructDeclaration = SpecifierQualifierList StructDeclaratorList ? | StaticAssertDeclaration
  SpecifierQualifierList = ( TypeSpecifier | TypeQualifier ) *
  StructDeclaratorList = StructDeclarator ( ',' StructDeclarator ) *
  StructDeclarator = Declarator ( ':' Expr.Const ) ? | ':' Expr.Const
  InitDeclaratorList = InitDeclarator ( ',' InitDeclarator ) *
  InitDeclarator = Declarator ( '=' Initializer ) ?
  Declarator = Pointer ? DirectDeclarator
  AbstractDeclarator = Pointer DirectAbstractDeclarator ? | DirectAbstractDeclarator
  DirectAbstractDeclarator = ( '(' AbstractDeclarator ')' | DirectAbstractDeclaratorExt ) DirectAbstractDeclaratorExt *
  DirectAbstractDeclaratorExt = CommonDeclaratorExt
  Pointer = '*' TypeQualifier * Pointer ?
  TypeQualifier = ( 'const' | 'volatile' | 'restrict' | '_Atomic' )
  DirectDeclarator = ( identifier | '(' Declarator ')' ) DirectDeclaratorExt *
  DirectDeclaratorExt = ( CommonDeclaratorExt
                        | '(' IdentifierList ')'
                        | '[' TypeQualifier * '*' ']'
                        )
  CommonDeclaratorExt = alias ( '[' ']'
                              | '[' '*' ']'
                              | '[' 'static' TypeQualifier * Expr ']'
                              | '[' 'static' Expr ']'
                              | '[' TypeQualifier * 'static' Expr ']'
                              | '[' TypeQualifier * Expr ']'
                              | '[' TypeQualifier * ']'
                              | '[' Expr ']'
                              | '(' ParameterTypeList ')'
                              | '(' ')'
                              )
  ParameterTypeList = ParameterList ( ',' '...' ) ?
  ParameterList = ParameterDeclaration ( ',' ParameterDeclaration ) *
  ParameterDeclaration = DeclarationSpecifiers ( Declarator | AbstractDeclarator ) ?
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
                     postfix_nest '(' ')' '[' ']' \
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
      Comma   = ltr  infix ','
    Atom = number | string + | identifier
    Const = Expr # not allowed to be Expr.Comma or Expr.Assign
)'";

  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t);
}
