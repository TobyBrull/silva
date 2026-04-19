#pragma once

#include "canopy/types.hpp"

#include "syntax/seed_interpreter.hpp"

namespace silva::cedar {
  const string_view_t seed_str = R"'(
tokenizer Cedar:
  include tokenizer FreeForm

language Cedar:
  ⊙ = External *
  External = FuncDef | Decl

  FuncDef = TypeSpecs Declarator PostDecl Compound
  Decl = TypeSpecs InitDeclList ? PostDecl ';'
  InitDeclList = InitDecl ( ',' InitDecl ) *
  InitDecl = Declarator PostDecl ( '=' Initializer ) ?

  TypeSpecs = NonTypeSpec * TypeCore NonTypeSpec *
  TypeCore = KeywordType | TypedefName
  TypedefName = identifier
  KeywordType = StructUnion | Enum | BasicType +
  BasicType = ( 'void' | 'char' | 'short' | 'int' | 'long' | 'float' | 'double' | 'signed' | 'unsigned' | '_Bool' | '_Complex' | '_Imaginary' )
  StructUnion = ( 'struct' | 'union' ) identifier ? ( '{' StructMember * '}' ) ?
  StructMember = TypeSpecs StructDeclaratorList ? PostDecl ';'
  StructDeclaratorList = StructDeclarator ( ',' StructDeclarator ) *
  StructDeclarator = Declarator PostDecl ( ':' Expr ) ? | ':' Expr
  Enum = 'enum' identifier ? ( '{' Enumerator ( ',' Enumerator ) * ',' ? '}' ) ?
  Enumerator = identifier ( '=' Expr ) ?

  NonTypeSpec = StorageClass | TypeQual | FuncSpec | AttrSpec | AsmSpec | Extension
  StorageClass = ( 'typedef' | 'extern' | 'static' | 'auto' | 'register' | '_Thread_local' | '__thread' )
  TypeQual = ( 'const' | 'volatile' | 'restrict' | '_Atomic' | '__const' | '__volatile' | '__restrict' | '__restrict__' )
  FuncSpec = ( 'inline' | '__inline' | '__inline__' | '_Noreturn' | '__forceinline' )
  Extension = '__extension__'

  AttrSpec = ( '__attribute__' | '__attribute' ) '(' AttrItem * ')'
  AttrItem = Nested | Plain
  Nested = '(' AttrItem * ')'
  Plain = not '(' not ')' any

  AsmSpec = ( '__asm__' | '__asm' | 'asm' ) '(' string + ')'

  PostDecl = ( AttrSpec | AsmSpec ) *

  Declarator = Pointer + DirectDeclarator ? | DirectDeclarator
  Pointer = ( '*' | '**' | '***' | '****' ) NonTypeSpec *
  DirectDeclarator = DDStart DDSuffix * | DDSuffix +
  DDStart = ParenDD | IdentDD
  IdentDD = identifier
  ParenDD = '(' Declarator ')'
  DDSuffix = ArraySuffix | FuncSuffix
  ArraySuffix = '[' NonTypeSpec * Expr ? ']'
  FuncSuffix = '(' ParamList ? ')'
  ParamList = ParamOrDots ( ',' ParamOrDots ) *
  ParamOrDots = ParamDecl | Ellipsis
  Ellipsis = '...'
  ParamDecl = TypeSpecs Declarator ?

  Initializer = InitList | Expr
  InitList = '{' Init ( ',' Init ) * ',' ? '}'
  Init = Designator * Initializer
  Designator = '.' identifier | '[' Expr ']'

  Compound = '{' BlockItem * '}'
  BlockItem = Stmt | Decl

  Stmt = ( If | While | For | DoWhile | Switch | Case | Default | Goto | Continue | Break | Return | Label | Compound | ExprStmt )
  If = 'if' '(' Expr ')' Stmt ( 'else' Stmt ) ?
  While = 'while' '(' Expr ')' Stmt
  DoWhile = 'do' Stmt 'while' '(' Expr ')' ';'
  For = 'for' '(' ForInit Expr ? ';' Expr ? ')' Stmt
  ForInit = Decl | ExprStmt
  Switch = 'switch' '(' Expr ')' Stmt
  Case = 'case' Expr ':' Stmt
  Default = 'default' ':' Stmt
  Goto = 'goto' identifier ';'
  Continue = 'continue' ';'
  Break = 'break' ';'
  Return = 'return' Expr ? ';'
  Label = identifier ':' Stmt
  ExprStmt = Expr ? ';'

  Expr:
    ⊙ = axe Atom
      Unary   = rtl prefix '!' '~' '-' '+' '*' '&' '++' '--'
      Mult    = ltr infix '*' '/' '%'
      Add     = ltr infix '+' '-'
      Shift   = ltr infix '<<' '>>'
      Rel     = ltr infix '<' '>' '<=' '>='
      Eq      = ltr infix '==' '!='
      BitAnd  = ltr infix '&'
      BitXor  = ltr infix '^'
      BitOr   = ltr infix '|'
      LogAnd  = ltr infix '&&'
      LogOr   = ltr infix '||'
      Tern    = rtl ternary '?' ':'
      Assign  = rtl infix '=' '+=' '-=' '*=' '/=' '%=' '<<=' '>>=' '&=' '^=' '|='
    Atom = AtomBase Suffix *
    AtomBase = SizeofType | SizeofExpr | CastX | CastT | Paren | Literal | Ident
    SizeofType = SizeofPrefix TypeSpecs Pointer * ')'
    SizeofPrefix = 'sizeof' '('
    SizeofExpr = 'sizeof' Atom
    CastX = CastPrefix Atom
    CastT = CastPrefix
    CastPrefix = OpenParen TypeSpecs Pointer * ')'
    OpenParen = '('
    Paren = '(' Expr ')'
    Literal = number | string +
    Ident = identifier
    Suffix = Call | Index | Dot | Arrow | PostIncDec
    Call = '(' ArgList ? ')'
    Index = '[' Expr ']'
    Dot = '.' identifier
    Arrow = '->' identifier
    PostIncDec = '++' | '--'
    ArgList = Expr ( ',' Expr ) *
)'";

  unique_ptr_t<seed::interpreter_t> seed_interpreter(syntax_farm_ptr_t);
}
