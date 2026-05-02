#pragma once

#include "canopy/types.hpp"

#include "syntax/seed_interpreter.hpp"

namespace silva::cedar {

  // Translation of https://www.quut.com/c/ANSI-C-grammar-y.html.
  //
  const string_view_t seed_str = R"'(
tokenizer Cedar:
  operator = [ '...' '.' ';' ':' '*=' '*' '/=' '/' '%=' '%' '~' '++' '+=' '+' '--' '-=' '->' '-'
               '!=' '!' '==' '=' '<<=' '<<' '<=' '<' '>>=' '>>' '>=' '>'
               '&&' '&=' '&' '||' '|=' '|' '^=' '^' '?' ',' ]
  include tokenizer FreeForm

language Cedar:
  ⊙ = Declaration *
  Declaration:
    ⊙ = Specifiers Init ? ( ';' | Stmt.Compound ) \
      | StaticAssert
    Specifiers = ( StorageClassSpecifier | FunctionSpecifier | AlignmentSpecifier | Type.Qualifier ) * Type.Specifier
    StorageClassSpecifier = ( 'typedef' | 'extern' | 'static' | '_Thread_local' | 'auto' | 'register' )
    FunctionSpecifier = 'inline' | '_Noreturn'
    AlignmentSpecifier = ( '_Alignas' | 'alignas' ) '(' ( Type.Name | Expr.Const ) ')'
    StaticAssert = '_Static_assert' '(' Expr.Const ',' string ')' ';'

    ParameterList = ParameterDeclaration ( ',' ParameterDeclaration ) *
    ParameterDeclaration = Specifiers ( Declarator | Declarator.Abstract ) ? | '...'
    IdentifierList = identifier ( ',' identifier ) *

    Designation = Designator + '='
    Designator = '.' identifier | '[' Expr.Const ']'

    Init = Declarator ( '=' Initializer ) ?
    Initializer = '{' InitializerList ',' ? '}' | Expr
    InitializerList = Designation ? Initializer ( ',' Designation ? Initializer ) *

  Declarator:
    ⊙ = Pointer ? Direct
    Abstract = Pointer DirectAbstract ? | DirectAbstract
    DirectAbstract = ( '(' Abstract ')' | DirectAbstractExt ) DirectAbstractExt *
    DirectAbstractExt = CommonExt
    Pointer = '*' Type.Qualifier * Pointer ?
    Direct = ( identifier | '(' Declarator ')' ) DirectExt *
    DirectExt = ( CommonExt | '(' Declaration.IdentifierList ')' )
    CommonExt = alias ( '[' Type.Qualifier * Expr ? ']'
                      | '(' Declaration.ParameterList ? ')'
                      )


  Type:
    IntSpecifier = ( 'signed' | 'unsigned' ) ? ( 'char' | 'short' | 'int' | 'long' )
    BuiltinSpecifier = 'float' | 'double' | '_Bool' | '_Complex' | '_Imaginary'
    Specifier = ( 'void'
                | IntSpecifier
                | BuiltinSpecifier
                | AtomicTypeSpecifier
                | StructOrUnionSpecifier
                | EnumSpecifier
                | TypedefName
                )
    TypedefName = identifier # TODO: should only match identifiers that have previously been typedef'ed.
    Qualifier = ( 'const' | 'volatile' | 'restrict' | '_Atomic' )
    SpecifierQualifierList = Type.Qualifier * Type.Specifier

    EnumSpecifier = 'enum' identifier ? '{' Enumerator ( ',' Enumerator ) * ',' ? '}'
    Enumerator = EnumerationConstant ( '=' Expr.Const ) ?
    Name = SpecifierQualifierList Declarator.Abstract ?

    EnumerationConstant = identifier

    AtomicTypeSpecifier = '_Atomic' '(' Name ')'
    StructOrUnionSpecifier = ( 'struct' | 'union' ) identifier ? ( '{' StructMemberDecl * '}' ) ?
    StructMemberDecl = Type.SpecifierQualifierList StructMemberDeclaratorList ? ';' | Declaration.StaticAssert
    StructMemberDeclaratorList = StructMemberDeclarator ( ',' StructMemberDeclarator ) *
    StructMemberDeclarator = Declarator ( ':' Expr.Const ) ? | ':' Expr.Const

  Stmt:
    ⊙ = ( Label | Case | Default
        | Compound
        | ExprStmt
        | If | Switch
        | While | DoWhile | For
        | Goto | Continue | Break | Return
        )
    Label = identifier ':' Stmt
    Case = 'case' Expr.Const ':' Stmt
    Default = 'default' ':' Stmt
    Compound = '{' ( Stmt | Declaration ) * '}'
    ExprStmt = Expr ? ';'
    If = 'if' '(' Expr ')' Stmt ( 'else' Stmt ) ?
    Switch = 'switch' '(' Expr ')' Stmt
    While = 'while' '(' Expr ')' Stmt
    DoWhile = 'do' Stmt 'while' '(' Expr ')' ';'
    For = 'for' '(' ( Declaration | ExprStmt ) Expr ? ';' Expr ? ')' Stmt
    Goto = 'goto' identifier ';'
    Continue = 'continue' ';'
    Break = 'break' ';'
    Return = 'return' Expr ? ';'

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
    Atom = number | string + | identifier
    ExprOrNone = Expr | None
    Const = Expr # not allowed to be Expr.Comma or Expr.Assign
)'";

  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t);
}
