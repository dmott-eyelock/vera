//
// Copyright (C) 2006-2007 Maciej Sobczak
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//

#include "Statements.h"
#include "StatementOfIf.h"
#include "StatementOfElse.h"
#include "StatementOfForLoop.h"
#include "StatementOfWhileLoop.h"
#include "StatementOfTryCatches.h"
#include "StatementOfDoWhileLoop.h"
#include "StatementOfSwitch.h"
#include "StatementOfCase.h"
#include "StatementOfCatch.h"
#include "StatementOfNamespace.h"
#include "StatementOfStruct.h"
#include "StatementOfAccessModifiers.h"
#include "StatementOfDefault.h"
#include "StatementOfOperatorTernario.h"

#include "IsTokenWithName.h"
#include <functional>
#include <boost/algorithm/string/case_conv.hpp>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>
#include <cctype>

#define LEFTPAREN_TOKEN_NAME  "leftparen"
#define RIGHTPAREN_TOKEN_NAME  "rightparen"
#define LEFTBRACE_TOKEN_NAME  "leftbrace"
#define RIGHTBRACE_TOKEN_NAME  "rightbrace"

#define IS_EQUAL_BREAK(left, right) \
  {\
    if (left == right) \
    { \
      break;\
    }\
  }
#define IS_EQUAL_RETURN(left, right) \
  {\
    if (left == right) \
    { \
      return;\
    }\
  }
#define IS_EQUAL_RETURN_FALSE(left, right) \
  {\
    if (left == right) \
    { \
      return false;\
    }\
  }

namespace
{
/**
 * @brief Selects the best strategy for the given token.
 */
class StrategySelector
{
  public:
    /**
     * @brief Factory method type.
     */
    typedef bool (*FactoryMethod)(Vera::Structures::Statement& statement,
        Vera::Structures::Tokens::TokenSequence::const_iterator& it,
        Vera::Structures::Tokens::TokenSequence::const_iterator& end);

  public:
    /**
     * @brief Initializes all the supported strategies.
     */
    StrategySelector()
    {
      factories_[boost::wave::T_IF] = &Vera::Structures::StatementOfIf::create;
      factories_[boost::wave::T_FOR] = &Vera::Structures::StatementOfForLoop::create;
      factories_[boost::wave::T_TRY] = &Vera::Structures::StatementOfTryCatches::create;
      factories_[boost::wave::T_DO] = &Vera::Structures::StatementOfDoWhileLoop::create;
      factories_[boost::wave::T_CATCH]= &Vera::Structures::StatementOfCatch::create;
      factories_[boost::wave::T_ELSE] = &Vera::Structures::StatementOfElse::create;
      factories_[boost::wave::T_CASE] = &Vera::Structures::StatementOfCase::create;
      factories_[boost::wave::T_DEFAULT] = &Vera::Structures::StatementOfDefault::create;
      factories_[boost::wave::T_SWITCH] = &Vera::Structures::StatementOfSwitch::create;
      factories_[boost::wave::T_WHILE] = &Vera::Structures::StatementOfWhileLoop::create;
      factories_[boost::wave::T_NAMESPACE] = &Vera::Structures::StatementOfNamespace::create;
      factories_[boost::wave::T_STRUCT] = &Vera::Structures::StatementOfStruct::create;
      factories_[boost::wave::T_PUBLIC] = &Vera::Structures::StatementOfAccessModifiers::create;
      factories_[boost::wave::T_PROTECTED] = &Vera::Structures::StatementOfAccessModifiers::create;
      factories_[boost::wave::T_PRIVATE] = &Vera::Structures::StatementOfAccessModifiers::create;
      factories_[boost::wave::T_QUESTION_MARK] = &Vera::Structures::StatementOfOperatorTernario::create;
      factories_[boost::wave::T_COLON] = &Vera::Structures::StatementOfOperatorTernario::create;
    }

    /**
     * @brief Gets the adequate factory method for the given token.
     *
     * @param id Defines the identifier of the related token.
     * @return The adequate factory method  for the given id.
     */
    FactoryMethod operator() (boost::wave::token_id id)
    {
      if (factories_.find(id) == factories_.end())
        throw;

      return *factories_[id];
    }

    /**
     * @brief  Determines if there's a factory method for the given token.
     *
     * @param id Defines the identifier of the related token.
     * @return True if there's a factory method. Otherwise false.
     */
    bool isHandled(boost::wave::token_id id)
    {
      return factories_.find(id) != factories_.end();
    }

  private:

    /**
      * @brief The factory methods map type.
      */
     typedef std::map<boost::wave::token_id, FactoryMethod> Factories;

  private:
     Factories factories_;
} builders_;

/**
 * @brief Determines the most adequate method to verify the syntax of
 * the given statements.
 */
class SelectorOfVerifiers
{
  public:
    /**
     * @brief IsValidMethod type.
     */
    typedef bool (*IsValidMethod)(Vera::Structures::Tokens::TokenSequence::const_iterator it,
        Vera::Structures::Tokens::TokenSequence::const_iterator end);

  public:
    /**
     * @brief Initializes the verifiers for the supported strategies.
     */
    SelectorOfVerifiers ()
    {
      verifiers_[boost::wave::T_IF] = &Vera::Structures::StatementOfIf::isValid;
      verifiers_[boost::wave::T_FOR] = &Vera::Structures::StatementOfForLoop::isValid;
      verifiers_[boost::wave::T_TRY] = &Vera::Structures::StatementOfTryCatches::isValid;
      verifiers_[boost::wave::T_DO] = &Vera::Structures::StatementOfDoWhileLoop::isValid;
      verifiers_[boost::wave::T_CATCH] = &Vera::Structures::StatementOfCatch::isValid;
      verifiers_[boost::wave::T_ELSE] = &Vera::Structures::StatementOfElse::isValid;
      verifiers_[boost::wave::T_CASE] = &Vera::Structures::StatementOfCase::isValid;
      verifiers_[boost::wave::T_DEFAULT] = &Vera::Structures::StatementOfDefault::isValid;
      verifiers_[boost::wave::T_SWITCH] = &Vera::Structures::StatementOfSwitch::isValid;
      verifiers_[boost::wave::T_WHILE] = &Vera::Structures::StatementOfWhileLoop::isValid;
      verifiers_[boost::wave::T_NAMESPACE] = &Vera::Structures::StatementOfNamespace::isValid;
      verifiers_[boost::wave::T_STRUCT] = &Vera::Structures::StatementOfStruct::isValid;
      verifiers_[boost::wave::T_PUBLIC] = &Vera::Structures::StatementOfAccessModifiers::isValid;
      verifiers_[boost::wave::T_PROTECTED] = &Vera::Structures::StatementOfAccessModifiers::isValid;
      verifiers_[boost::wave::T_PRIVATE] = &Vera::Structures::StatementOfAccessModifiers::isValid;
      verifiers_[boost::wave::T_QUESTION_MARK] = &Vera::Structures::StatementOfOperatorTernario::isValid;
      verifiers_[boost::wave::T_COLON] = &Vera::Structures::StatementOfOperatorTernario::isValid;
    }

    /**
     * @brief Gets the adequate verifier method for the given token.
     *
     * @param id Defines the identifier of the related token.
     * @return The adequate verifier method for the given id.
     */
    IsValidMethod operator() (boost::wave::token_id id)
    {
      if (isHandled(id) == false)
        throw;

      return *verifiers_[id];
    }

    /**
     * @brief Determines if there's a valid strategy for the given token.
     *
     * @param id Defines the identifier of the related token.
     * @return True if there's a valid strategy. Otherwise false.
     */
    bool isHandled(boost::wave::token_id id)
    {
      return verifiers_.find(id) != verifiers_.end();
    }

  private:

     /**
       * @brief The isValid methods map type.
       */
      typedef std::map<boost::wave::token_id, IsValidMethod> IsValidMapType;

  private:
      IsValidMapType verifiers_;
} isValid_;

} /*unname namespace */

namespace Vera
{
namespace Structures
{

using namespace underlyImpl;

namespace //unname
{

/**
 * @brief Gets the enumerator id associated to the token.
 *
 * @param token The reference to the token instance.
 * @return The id assigned to the token.
 */
static boost::wave::token_id
getTokenId(const Token& token);

/**
 *  @brief Binary function object class whose call returns whether a item with id given.
 */
class IsTokenWithId
: public std::unary_function<const Token, bool>
{
  public:

    /**
     * @brief Initializes a new instance of the IsTokenEqual class.
     * @param id The lexer id desired.
     */
    IsTokenWithId(boost::wave::token_id id)
    : id_(id)
    {
    }

    /**
     * @brief Member function that returns true if the element is equal to the given token.
     *
     * @param token The token to be compare
     *
     * @return True if the item is equal to the given token, otherwise, false.
     */
    result_type operator()(argument_type item) const
    {
      return getTokenId(item) == id_;
    }

  private:

    boost::wave::token_id id_;

};

#define returnIdWheterTokenMatch(TOKEN, NAME) \
{ \
  std::string tokenName = #TOKEN; \
  boost::algorithm::to_lower(tokenName);\
   \
  if (tokenName.compare(NAME) == 0) \
  { \
    return boost::wave::T_##TOKEN; \
  }\
}

static boost::wave::token_id
getTokenId(const Token& token)
{
  std::string name = token.name_;

  returnIdWheterTokenMatch(ALIGNAS, name)
  returnIdWheterTokenMatch(ALIGNOF, name)
  returnIdWheterTokenMatch(AND, name)
  returnIdWheterTokenMatch(AND_ALT, name)
  returnIdWheterTokenMatch(ANDAND, name)
  returnIdWheterTokenMatch(ANDAND_ALT, name)
  returnIdWheterTokenMatch(ANDASSIGN, name)
  returnIdWheterTokenMatch(ANDASSIGN_ALT, name)
  returnIdWheterTokenMatch(ANY, name)
  returnIdWheterTokenMatch(ANY_TRIGRAPH, name)
  returnIdWheterTokenMatch(ARROW, name)
  returnIdWheterTokenMatch(ARROWSTAR, name)
  returnIdWheterTokenMatch(ASM, name)
  returnIdWheterTokenMatch(ASSIGN, name)
  returnIdWheterTokenMatch(AUTO, name)
  returnIdWheterTokenMatch(BOOL, name)
  returnIdWheterTokenMatch(BREAK, name)
  returnIdWheterTokenMatch(CASE, name)
  returnIdWheterTokenMatch(CATCH, name)
  returnIdWheterTokenMatch(CCOMMENT, name)
  returnIdWheterTokenMatch(CHAR, name)
  returnIdWheterTokenMatch(CHAR16_T, name)
  returnIdWheterTokenMatch(CHAR32_T, name)
  returnIdWheterTokenMatch(CHARLIT, name)
  returnIdWheterTokenMatch(CLASS, name)
  returnIdWheterTokenMatch(COLON, name)
  returnIdWheterTokenMatch(COLON_COLON, name)
  returnIdWheterTokenMatch(COMMA, name)
  returnIdWheterTokenMatch(COMPL, name)
  returnIdWheterTokenMatch(COMPL_ALT, name)
  returnIdWheterTokenMatch(COMPL_TRIGRAPH, name)
  returnIdWheterTokenMatch(CONST, name)
  returnIdWheterTokenMatch(CONSTCAST, name)
  returnIdWheterTokenMatch(CONSTEXPR, name)
  returnIdWheterTokenMatch(CONTINUE, name)
  returnIdWheterTokenMatch(CONTLINE, name)
  returnIdWheterTokenMatch(CPPCOMMENT, name)
  returnIdWheterTokenMatch(DECIMALINT, name)
  returnIdWheterTokenMatch(DECLTYPE, name)
  returnIdWheterTokenMatch(DEFAULT, name)
  returnIdWheterTokenMatch(DELETE, name)
  returnIdWheterTokenMatch(DIVIDE, name)
  returnIdWheterTokenMatch(DIVIDEASSIGN, name)
  returnIdWheterTokenMatch(DO, name)
  returnIdWheterTokenMatch(DOT, name)
  returnIdWheterTokenMatch(DOTSTAR, name)
  returnIdWheterTokenMatch(DOUBLE, name)
  returnIdWheterTokenMatch(DYNAMICCAST, name)
  returnIdWheterTokenMatch(ELLIPSIS, name)
  returnIdWheterTokenMatch(ELSE, name)
  returnIdWheterTokenMatch(ENUM, name)
  returnIdWheterTokenMatch(EOF, name)
  returnIdWheterTokenMatch(EOI, name)
  returnIdWheterTokenMatch(EQUAL, name)
  returnIdWheterTokenMatch(EXPLICIT, name)
  returnIdWheterTokenMatch(EXPORT, name)
  returnIdWheterTokenMatch(EXTERN, name)
  returnIdWheterTokenMatch(FALSE, name)
  returnIdWheterTokenMatch(FIRST_TOKEN, name)
  returnIdWheterTokenMatch(FIXEDPOINTLIT, name)
  returnIdWheterTokenMatch(FLOAT, name)
  returnIdWheterTokenMatch(FLOATLIT, name)
  returnIdWheterTokenMatch(FOR, name)
  returnIdWheterTokenMatch(FRIEND, name)
  returnIdWheterTokenMatch(GENERATEDNEWLINE, name)
  returnIdWheterTokenMatch(GOTO, name)
  returnIdWheterTokenMatch(GREATER, name)
  returnIdWheterTokenMatch(GREATEREQUAL, name)
  returnIdWheterTokenMatch(HEXAINT, name)
  returnIdWheterTokenMatch(IDENTIFIER, name)
  returnIdWheterTokenMatch(IF, name)
  returnIdWheterTokenMatch(IMPORT, name)
  returnIdWheterTokenMatch(INLINE, name)
  returnIdWheterTokenMatch(INT, name)
  returnIdWheterTokenMatch(INTLIT, name)
  returnIdWheterTokenMatch(LEFTBRACE, name)
  returnIdWheterTokenMatch(LEFTBRACE_ALT, name)
  returnIdWheterTokenMatch(LEFTBRACE_TRIGRAPH, name)
  returnIdWheterTokenMatch(LEFTBRACKET, name)
  returnIdWheterTokenMatch(LEFTBRACKET_ALT, name)
  returnIdWheterTokenMatch(LEFTBRACKET_TRIGRAPH, name)
  returnIdWheterTokenMatch(LEFTPAREN, name)
  returnIdWheterTokenMatch(LESS, name)
  returnIdWheterTokenMatch(LESSEQUAL, name)
  returnIdWheterTokenMatch(LONG, name)
  returnIdWheterTokenMatch(LONGINTLIT, name)
  returnIdWheterTokenMatch(MINUS, name)
  returnIdWheterTokenMatch(MINUSASSIGN, name)
  returnIdWheterTokenMatch(MINUSMINUS, name)
  returnIdWheterTokenMatch(MSEXT_ASM, name)
  returnIdWheterTokenMatch(MSEXT_BASED, name)
  returnIdWheterTokenMatch(MSEXT_CDECL, name)
  returnIdWheterTokenMatch(MSEXT_DECLSPEC, name)
  returnIdWheterTokenMatch(MSEXT_EXCEPT, name)
  returnIdWheterTokenMatch(MSEXT_FASTCALL, name)
  returnIdWheterTokenMatch(MSEXT_FINALLY, name)
  returnIdWheterTokenMatch(MSEXT_INLINE, name)
  returnIdWheterTokenMatch(MSEXT_INT16, name)
  returnIdWheterTokenMatch(MSEXT_INT32, name)
  returnIdWheterTokenMatch(MSEXT_INT64, name)
  returnIdWheterTokenMatch(MSEXT_INT8, name)
  returnIdWheterTokenMatch(MSEXT_LEAVE, name)
  returnIdWheterTokenMatch(MSEXT_PP_ENDREGION, name)
  returnIdWheterTokenMatch(MSEXT_PP_REGION, name)
  returnIdWheterTokenMatch(MSEXT_STDCALL, name)
  returnIdWheterTokenMatch(MSEXT_TRY, name)
  returnIdWheterTokenMatch(MUTABLE, name)
  returnIdWheterTokenMatch(NAMESPACE, name)
  returnIdWheterTokenMatch(NEW, name)
  returnIdWheterTokenMatch(NEWLINE, name)
  returnIdWheterTokenMatch(NOEXCEPT, name)
  returnIdWheterTokenMatch(NOT, name)
  returnIdWheterTokenMatch(NOT_ALT, name)
  returnIdWheterTokenMatch(NOTEQUAL, name)
  returnIdWheterTokenMatch(NOTEQUAL_ALT, name)
  returnIdWheterTokenMatch(NULLPTR, name)
  returnIdWheterTokenMatch(OCTALINT, name)
  returnIdWheterTokenMatch(OPERATOR, name)
  returnIdWheterTokenMatch(OR, name)
  returnIdWheterTokenMatch(OR_ALT, name)
  returnIdWheterTokenMatch(ORASSIGN, name)
  returnIdWheterTokenMatch(ORASSIGN_ALT, name)
  returnIdWheterTokenMatch(ORASSIGN_TRIGRAPH, name)
  returnIdWheterTokenMatch(OROR, name)
  returnIdWheterTokenMatch(OROR_ALT, name)
  returnIdWheterTokenMatch(OROR_TRIGRAPH, name)
  returnIdWheterTokenMatch(OR_TRIGRAPH, name)
  returnIdWheterTokenMatch(PERCENT, name)
  returnIdWheterTokenMatch(PERCENTASSIGN, name)
  returnIdWheterTokenMatch(PLUS, name)
  returnIdWheterTokenMatch(PLUSASSIGN, name)
  returnIdWheterTokenMatch(PLUSPLUS, name)
  returnIdWheterTokenMatch(POUND, name)
  returnIdWheterTokenMatch(POUND_ALT, name)
  returnIdWheterTokenMatch(POUND_POUND, name)
  returnIdWheterTokenMatch(POUND_POUND_ALT, name)
  returnIdWheterTokenMatch(POUND_POUND_TRIGRAPH, name)
  returnIdWheterTokenMatch(POUND_TRIGRAPH, name)
  returnIdWheterTokenMatch(PP_DEFINE, name)
  returnIdWheterTokenMatch(PP_ELIF, name)
  returnIdWheterTokenMatch(PP_ELSE, name)
  returnIdWheterTokenMatch(PP_ENDIF, name)
  returnIdWheterTokenMatch(PP_ERROR, name)
  returnIdWheterTokenMatch(PP_HHEADER, name)
  returnIdWheterTokenMatch(PP_HHEADER_NEXT, name)
  returnIdWheterTokenMatch(PP_IF, name)
  returnIdWheterTokenMatch(PP_IFDEF, name)
  returnIdWheterTokenMatch(PP_IFNDEF, name)
  returnIdWheterTokenMatch(PP_INCLUDE, name)
  returnIdWheterTokenMatch(PP_INCLUDE_NEXT, name)
  returnIdWheterTokenMatch(PP_LINE, name)
  returnIdWheterTokenMatch(PP_NUMBER, name)
  returnIdWheterTokenMatch(PP_PRAGMA, name)
  returnIdWheterTokenMatch(PP_QHEADER, name)
  returnIdWheterTokenMatch(PP_QHEADER_NEXT, name)
  returnIdWheterTokenMatch(PP_UNDEF, name)
  returnIdWheterTokenMatch(PP_WARNING, name)
  returnIdWheterTokenMatch(PRIVATE, name)
  returnIdWheterTokenMatch(PROTECTED, name)
  returnIdWheterTokenMatch(PUBLIC, name)
  returnIdWheterTokenMatch(QUESTION_MARK, name)
  returnIdWheterTokenMatch(RAWSTRINGLIT, name)
  returnIdWheterTokenMatch(REGISTER, name)
  returnIdWheterTokenMatch(REINTERPRETCAST, name)
  returnIdWheterTokenMatch(RETURN, name)
  returnIdWheterTokenMatch(RIGHTBRACE, name)
  returnIdWheterTokenMatch(RIGHTBRACE_ALT, name)
  returnIdWheterTokenMatch(RIGHTBRACE_TRIGRAPH, name)
  returnIdWheterTokenMatch(RIGHTBRACKET, name)
  returnIdWheterTokenMatch(RIGHTBRACKET_ALT, name)
  returnIdWheterTokenMatch(RIGHTBRACKET_TRIGRAPH, name)
  returnIdWheterTokenMatch(RIGHTPAREN, name)
  returnIdWheterTokenMatch(SEMICOLON, name)
  returnIdWheterTokenMatch(SHIFTLEFT, name)
  returnIdWheterTokenMatch(SHIFTLEFTASSIGN, name)
  returnIdWheterTokenMatch(SHIFTRIGHT, name)
  returnIdWheterTokenMatch(SHIFTRIGHTASSIGN, name)
  returnIdWheterTokenMatch(SHORT, name)
  returnIdWheterTokenMatch(SIGNED, name)
  returnIdWheterTokenMatch(SIZEOF, name)
  returnIdWheterTokenMatch(SPACE, name)
  returnIdWheterTokenMatch(SPACE2, name)
  returnIdWheterTokenMatch(STAR, name)
  returnIdWheterTokenMatch(STARASSIGN, name)
  returnIdWheterTokenMatch(STATIC, name)
  returnIdWheterTokenMatch(STATICASSERT, name)
  returnIdWheterTokenMatch(STATICCAST, name)
  returnIdWheterTokenMatch(STRINGLIT, name)
  returnIdWheterTokenMatch(STRUCT, name)
  returnIdWheterTokenMatch(SWITCH, name)
  returnIdWheterTokenMatch(TEMPLATE, name)
  returnIdWheterTokenMatch(THIS, name)
  returnIdWheterTokenMatch(THREADLOCAL, name)
  returnIdWheterTokenMatch(THROW, name)
  returnIdWheterTokenMatch(TRUE, name)
  returnIdWheterTokenMatch(TRY, name)
  returnIdWheterTokenMatch(TYPEDEF, name)
  returnIdWheterTokenMatch(TYPEID, name)
  returnIdWheterTokenMatch(TYPENAME, name)
  returnIdWheterTokenMatch(UNION, name)
  returnIdWheterTokenMatch(UNKNOWN, name)
  returnIdWheterTokenMatch(UNSIGNED, name)
  returnIdWheterTokenMatch(USING, name)
  returnIdWheterTokenMatch(VIRTUAL, name)
  returnIdWheterTokenMatch(VOID, name)
  returnIdWheterTokenMatch(VOLATILE, name)
  returnIdWheterTokenMatch(WCHART, name)
  returnIdWheterTokenMatch(WHILE, name)
  returnIdWheterTokenMatch(XOR, name)
  returnIdWheterTokenMatch(XOR_ALT, name)
  returnIdWheterTokenMatch(XORASSIGN, name)
  returnIdWheterTokenMatch(XORASSIGN_ALT, name)
  returnIdWheterTokenMatch(XORASSIGN_TRIGRAPH, name)
  returnIdWheterTokenMatch(XOR_TRIGRAPH, name)

  return boost::wave::T_UNKNOWN;
}

/**
 * @brief Adds a new statement to the statement collection associated to the parent statement.
 *
 * @param current
 * @return The reference to the new statement.
 */
Statement&
addStatement(Statement& current)
{
  current.statementSequence_.push_back(Statement());
  Statement& next = current.statementSequence_.back();
  current.childs_.push_back(Statement::TYPE_ITEM_STATEMENT);
  return next;
}

/**
 * @brief Parses the statements from the token list
 * and retrieves the related list of tokens.
 *
 * @param response [out] the reference to the statement
 * used to return the response.
 * @param it the reference to the const iterator with the
 * initial element of the statement.
 * @param end the of the list to be analyzed.
 */
void parseStatement(Statement& response,
  TokenSequenceConstIterator& it,
  TokenSequenceConstIterator& end);

/**
 * @brief Parses the statements from the token list
 * and retrieves the related list of tokens.
 * @param response [out] the reference to the statement
 * used to return the response.
 * @param collection token list to be analyzed.
 */
void parseStatement(Statement& response, Tokens::TokenSequence& collection);

/**
 * @brief TODO
 * @param response
 * @param it
 * @param end
 */
void parseStatementDefineType(Statement& response,
  TokenSequenceConstIterator& it,
  TokenSequenceConstIterator& end);

/**
 * @brief TODO
 */
Tokens::TokenSequence
extractTokens(TokenSequenceConstIterator& begin,
    TokenSequenceConstIterator& end, boost::wave::token_id id);

/**
 * @brief Parses recursively the statements from the
 * token list and retrieves the related list of tokens.
 *
 * @param response [out] the reference to the statement
 * used to return the response.
 * @param it the reference to the const iterator with the
 * initial element of the statement.
 * @param end the of the list to be analyzed.
 */
void recursiveParseStatement(Statement& response,
  TokenSequenceConstIterator& it,
  TokenSequenceConstIterator& end)
{
  while (it != end)
  {
    const struct Token& token = *it;
    boost::wave::token_id id = getTokenId(token);

    IS_EQUAL_BREAK(id, boost::wave::T_EOF)

    if ((id == boost::wave::T_QUESTION_MARK || id == boost::wave::T_COLON) && isValid_(id)(it, end) == true)
    {
      parseStatement(response, it, end);
      IS_EQUAL_BREAK(it, end);
      continue;
    }

    if (builders_.isHandled(id) && isValid_(id)(it, end) == true)
    {
      parseStatement(response, it, end);
      IS_EQUAL_BREAK(it, end);
      ++it;
      continue;
    }

    if (id == boost::wave::T_EXTERN)
    {
      Statement& current = addStatement(response);
      current.push(*it);

      ++it;
      parseStatement(current, it, end);
      IS_EQUAL_BREAK(it, end);
      ++it; //TODO require continue?
      continue;
    }

    if (id == boost::wave::T_LEFTPAREN ||
        id == boost::wave::T_LEFTBRACE ||
        id == boost::wave::T_LEFTBRACKET)
    {
      Statement& current = addStatement(response);

      current.push(*it);
      ++it;

      TokenSequenceConstIterator itMatched = std::find_if(it,
        end,
        EndsWithCorrectPattern(id));

      recursiveParseStatement(current, it, itMatched);

      IS_EQUAL_BREAK(it, end);
      current.push(*it);
      ++it;
      IS_EQUAL_BREAK(it, end);
    }
    else
    {
      response.push(*it);
    }

    if (id == boost::wave::T_SEMICOLON)
    {
      //TODO break;
    }

    if (id == boost::wave::T_COLON)
    {
      break;
    }

    if (id == boost::wave::T_RIGHTPAREN)
    {
      break;
    }

    if (id == boost::wave::T_RIGHTBRACE)
    {
      break;
    }

    if (id == boost::wave::T_RIGHTBRACKET)
    {
      break;
    }

    if (id == boost::wave::T_PP_DEFINE)
    {
      parseStatementDefineType(response, it, end);
      IS_EQUAL_BREAK(it, end);
    }

    ++it;
  }
}

void parseStatementDefineType(Statement& response,
  TokenSequenceConstIterator& it,
  TokenSequenceConstIterator& end)
{
  response.push(*it);
  ++it;

  Statement& current = addStatement(response);

  Tokens::TokenSequence subCollection = extractTokens(it, end, boost::wave::T_NEWLINE);

  parseStatement(current, subCollection);
}

Tokens::TokenSequence
extractTokens(TokenSequenceConstIterator& begin,
    TokenSequenceConstIterator& end, boost::wave::token_id id)
{
  Tokens::TokenSequence response;

  TokenSequenceConstIterator itMatched = std::find_if(begin, end, IsTokenWithId(id));

  if (itMatched != end && itMatched != begin)
  {
    std::copy(begin,
      itMatched,
      std::back_inserter<Vera::Structures::Tokens::TokenSequence>(response));
  }

  return response;
}

void
parseStatement(Statement& response, Tokens::TokenSequence& collection)
{
  TokenSequenceConstIterator it = collection.begin();
  TokenSequenceConstIterator end = collection.end();


  parseStatement(response, it, end);
}

void parseStatement(Statement& response,
  TokenSequenceConstIterator& it,
  TokenSequenceConstIterator& end)
{
  while (it != end)
  {
    const struct Token& token = *it;

    boost::wave::token_id id = getTokenId(token);

    if (id == boost::wave::T_EOF)
    {
      break;
    }

    if ((id == boost::wave::T_QUESTION_MARK || id == boost::wave::T_COLON) && isValid_(id)(it, end) == true)
    {
      builders_(id)(response, it, end);
      IS_EQUAL_BREAK(it, end);
      continue;
    }

    if (builders_.isHandled(id) && isValid_(id)(it, end) == true)
    {
      builders_(id)(response, it, end);
      IS_EQUAL_BREAK(it, end);
      //++it;
      break;
    }

    if (id == boost::wave::T_LEFTPAREN ||
        id == boost::wave::T_LEFTPAREN ||
        id == boost::wave::T_LEFTBRACKET)
    {
      Statement& current = addStatement(response);

      current.push(*it);
      ++it;

      TokenSequenceConstIterator itMatched = std::find_if(it,
        end,
        EndsWithCorrectPattern(id));

      recursiveParseStatement(current, it, itMatched);

      IS_EQUAL_BREAK(it, end)
      current.push(*it);
      ++it;
      IS_EQUAL_BREAK(it, end)

      continue;
    }

//    if (id == boost::wave::T_LEFTBRACE ||
//      id == boost::wave::T_LEFTPAREN ||
//      id == boost::wave::T_LEFTBRACKET)
//    {
//      break;
//    }

    if (id == boost::wave::T_RIGHTPAREN)
    {
      response.push(*it);
      break;
    }

    if (id == boost::wave::T_RIGHTBRACKET)
    {
      response.push(*it);
      break;
    }

    if (id == boost::wave::T_COLON)
    {
      response.push(*it);
      break;
    }

    if (id == boost::wave::T_SEMICOLON)
    {
      response.push(*it);
      break;
    }

    if (id == boost::wave::T_PP_DEFINE)
    {
      parseStatementDefineType(response, it, end);
      break;
    }

    response.push(*it);
    ++it;
  }
}

} //unname namespace

bool
Statement::operator==(const Statement& statement) const
{

  bool isEqual = tokenSequence_.size() == statement.tokenSequence_.size();

  if (isEqual)
  {
    isEqual = std::equal(tokenSequence_.begin(),
      tokenSequence_.end(),
      statement.tokenSequence_.begin());
  }

  if (isEqual && statementSequence_.size() == statement.statementSequence_.size())
  {
    isEqual = std::equal(statementSequence_.begin(),
        statementSequence_.end(),
      statement.statementSequence_.begin());
  }

  return isEqual;
}


void
Statement::push(Token token)
{
  tokenSequence_.push_back(token);
  childs_.push_back(Statement::TYPE_ITEM_TOKEN);
}

Statement
StatementsBuilder::create(Token token, Tokens::TokenSequence& tokenCollection)
{
  Statement response;
  StatementsBuilder concreteBuilder(response);

  Tokens::TokenSequence::const_iterator it = tokenCollection.begin();
  Tokens::TokenSequence::const_iterator end = tokenCollection.end();

  it = std::find_if(it, end, IsTokenEqual(token));

  if (it != end)
  {
    parseStatement(response, it, end);
  }

  return response;
}

Statement&
StatementsBuilder::getCurrentStatement()
{
  return statement_;
}

void
StatementsBuilder::setCurrentStatement(Statement& current)
{
  statement_ = current;
}

Statement&
StatementsBuilder::add()
{
  return addStatement(statement_);
}

void
StatementsBuilder::builder(Statement& response,
  Tokens::TokenSequence::const_iterator& it,
  Tokens::TokenSequence::const_iterator& end)
{
  if (it == end)
  {
    throw StatementsError("Token Collection is empty.");
  }

  boost::wave::token_id id = getTokenId(*it);
  StatementsBuilder partial(response);


  if (isValid_.isHandled(id) == true)
  {
    if (isValid_(id)(it, end) == true)
    {
      //parseStatement(response, it, end);
      parseStatement(addStatement(response), it, end);
    }
    else
    {
      parseStatement(response, it, end);
      //parseStatement(addStatement(response), it, end);
    }
  }
  else
  {
    parseStatement(addStatement(response), it, end);
  }
}

StatementsBuilder::StatementsBuilder(Statement& statement)
: statement_(statement)
{
}

void
StatementsBuilder::addEachInvalidToken(Statement& current,
  Tokens::TokenSequence::const_iterator& it,
  Tokens::TokenSequence::const_iterator& end)
{
  Tokens::TokenSequence& tokenList = current.tokenSequence_;
  Tokens::TokenSequence::const_iterator itMatched = std::find_if(it,
    end,
    IsValidTokenForStatement());

  std::copy(it, itMatched, std::back_inserter<Vera::Structures::Tokens::TokenSequence>(tokenList));

  for (int i = 0; i < std::distance(it, itMatched); ++i)
  {
    current.childs_.push_back(Statement::TYPE_ITEM_TOKEN);
  }

  it += std::distance(it, itMatched);
}

bool
StatementsBuilder::parseArguments(Tokens::TokenSequence::const_iterator& it,
  Tokens::TokenSequence::const_iterator& end)
{
  if (it == end)
  {
    return false;
  }

  Tokens::TokenSequence::const_iterator itMatched = std::find_if(it,
     end,
     IsValidTokenForStatement());

  if (itMatched == end)
  {
      return false;
  }

  if (IsTokenWithId(boost::wave::T_LEFTPAREN)(*itMatched) == false)
  {
    return false;
  }

  addEachInvalidToken(getCurrentStatement(), it, end);

  Statement& current = add();

  current.push(*it);
  ++it;

  parseStatement(current, it, end);

  if (IsTokenWithId(boost::wave::T_RIGHTPAREN)(*it) == false)
  {
    return false;
  }

  return true;
}

void
StatementsBuilder::parseScope(Tokens::TokenSequence::const_iterator& it,
  Tokens::TokenSequence::const_iterator& end)
{
  IS_EQUAL_RETURN(it, end)

  Statement& current = add();
  addEachInvalidToken(current, it, end);

  if (IsTokenWithId(boost::wave::T_LEFTBRACE)(*it) == true)
  {
    current.push(*it);

    for (++it; it != end; ++it)
    {
      addEachInvalidToken(current, it, end);

      IS_EQUAL_BREAK(it, end)

      if (IsTokenWithId(boost::wave::T_RIGHTBRACE)(*it) == true)
      {
        current.push(*it);
        break;
      }

      if (IsTokenWithId(boost::wave::T_LEFTBRACE)(*it) == true)
      {
        StatementsBuilder(current).parseScope(it, end);
        continue;
      }

      builder(current, it, end);

      IS_EQUAL_BREAK(it, end)
    }
  }
  else
  {
    //builder(current, it, end);
    parseStatement(current, it, end);
  }
}

const
Tokens::TokenSequence&
StatementsBuilder::getTokens()
{
  return statement_.tokenSequence_;
}

void
StatementsBuilder::push(Token token)
{
  statement_.push(token);
}

bool
StatementsBuilder::parseHeritage(Tokens::TokenSequence::const_iterator& it,
  Tokens::TokenSequence::const_iterator& end)
{
  Statement& current = getCurrentStatement();
  if (IsTokenWithId(boost::wave::T_COLON)(*it) == false)
  {
    return false;
  }

  do
  {
    push(*it);
    ++it;
    IS_EQUAL_BREAK(it, end)

    addEachInvalidToken(current, it, end);

    if (IsTokenWithId(boost::wave::T_IDENTIFIER)(*it) == true ||
     IsTokenWithId(boost::wave::T_VIRTUAL)(*it) == true       ||
     IsTokenWithId(boost::wave::T_PRIVATE)(*it) == true       ||
     IsTokenWithId(boost::wave::T_PUBLIC)(*it) == true        ||
     IsTokenWithId(boost::wave::T_PROTECTED)(*it) == true)
    {
      StatementsBuilder partial(add());
      if (partial.parseArgument(it, end) == false)
      {
        return false;
      }

      IS_EQUAL_BREAK(it, end)
    }

  } while (IsTokenWithId(boost::wave::T_COMMA)(*it) == true);

  return true;
}

bool
StatementsBuilder::paramentersWithLastToken(
  Tokens::TokenSequence::const_iterator& it,
  Tokens::TokenSequence::const_iterator& end,
  boost::wave::token_id id_)
{
  if (it == end)
  {
    return false;
  }

  push(*it);
  ++it;
  IS_EQUAL_RETURN_FALSE(it, end)

  do
  {
    StatementsBuilder partial(add());
    partial.parseArgument(it, end);

    IS_EQUAL_BREAK(it, end)


    push(*it);

    if (IsTokenWithId(id_)(*it) == true)
    {
      break;
    }

    ++it;
  } while (it != end);

  return true;
}

bool
StatementsBuilder::parseArgument(
    Tokens::TokenSequence::const_iterator& it,
    Tokens::TokenSequence::const_iterator& end)
{
  IS_EQUAL_RETURN_FALSE(it, end)

  push(*it);
  ++it;

  while (it != end)
  {
    if (IsTokenWithId(boost::wave::T_LEFTPAREN)(*it) == true)
    {
      StatementsBuilder partial(add());
      partial.paramentersWithLastToken(it, end, boost::wave::T_RIGHTPAREN);
      continue;
    }

    if (IsTokenWithId(boost::wave::T_LEFTBRACKET)(*it) == true)
    {
      StatementsBuilder partial(add());
      partial.paramentersWithLastToken(it, end, boost::wave::T_RIGHTBRACKET);
      continue;
    }

    if (IsTokenWithId(boost::wave::T_LESS)(*it) == true)
    {
      StatementsBuilder partial(add());
      partial.paramentersWithLastToken(it, end, boost::wave::T_GREATER);
      IS_EQUAL_BREAK(it, end);
      ++it;
      continue;
    }

    if (IsTokenWithId(boost::wave::T_RIGHTPAREN)(*it) == true)
    {
      push(*it);
      break;
    }

    if (IsTokenWithId(boost::wave::T_RIGHTBRACKET)(*it) == true)
    {
      push(*it);
      break;
    }

    if (IsTokenWithId(boost::wave::T_GREATER)(*it) == true)
    {
      //current.push(*it);
      break;
    }

    if (IsTokenWithId(boost::wave::T_LEFTBRACE)(*it) == true ||
        IsTokenWithId(boost::wave::T_SEMICOLON)(*it) == true ||
        IsTokenWithId(boost::wave::T_COMMA)(*it) == true)
    {
      break;
    }

    push(*it);
    ++it;
  }

  return true;
}



bool
StatementsBuilder::parseVariablesFromScopeToSemicolon(Tokens::TokenSequence::const_iterator& it,
    Tokens::TokenSequence::const_iterator& end, std::vector<boost::wave::token_id>& finishTypeList)
{
  IS_EQUAL_RETURN_FALSE(it, end)

  Statement& current = getCurrentStatement();

  //push(*it);
  //++it;
  do
  {
    IS_EQUAL_BREAK(it, end)
    addEachInvalidToken(current, it, end);
    IS_EQUAL_BREAK(it, end)

    boost::wave::token_id id = getTokenId(*it);

    if (id == boost::wave::T_COMMA)
    {
      push(*it);
      ++it;

      IS_EQUAL_BREAK(it, end)
      addEachInvalidToken(current, it, end);
      IS_EQUAL_BREAK(it, end)

      id = getTokenId(*it);
    }
    else
    {
      bool finish = std::find(finishTypeList.begin(),
        finishTypeList.end(),
        id) != finishTypeList.end();

      if (finish)
      {
        break;
      }
    }

    IS_EQUAL_BREAK(it, end)

    if (id != boost::wave::T_COMMA)
    {
      StatementsBuilder partial(add());

      if (partial.parseVariableDeclaration(it, end, finishTypeList) == false)
      {
        return false;
      }

      IS_EQUAL_BREAK(it, end)
    }

  } while (IsTokenWithId(boost::wave::T_COMMA)(*it) == true);

  return true;
}



bool
StatementsBuilder::parseVariableDeclaration(Tokens::TokenSequence::const_iterator& it,
    Tokens::TokenSequence::const_iterator& end,
    std::vector<boost::wave::token_id>& finishTypeList)
{
  IS_EQUAL_RETURN_FALSE(it, end)
  std::map<boost::wave::token_id, boost::wave::token_id> tokens;

  tokens[boost::wave::T_LEFTBRACE] = boost::wave::T_RIGHTBRACE;
  tokens[boost::wave::T_LEFTPAREN] = boost::wave::T_RIGHTPAREN;
  tokens[boost::wave::T_LEFTBRACKET] = boost::wave::T_RIGHTBRACKET;
  tokens[boost::wave::T_LESS] = boost::wave::T_GREATER;

  //push(*it);
  //++it;

  while (it != end)
  {
    boost::wave::token_id id = getTokenId(*it);

    bool finish = std::find(finishTypeList.begin(), finishTypeList.end(), id) != finishTypeList.end();

    if (finish)
      break;

    if (id == boost::wave::T_ASSIGN)
    {
      StatementsBuilder partial(add());

      partial.push(*it);
      ++it;
      IS_EQUAL_BREAK(it, end)

      if (partial.parseVariableDeclaration(it, end, finishTypeList) == false)
      {
        return false;
      }

      continue;
    }

    if (id == boost::wave::T_LEFTBRACE ||
        id == boost::wave::T_LEFTPAREN ||
        id == boost::wave::T_LESS      ||
        id == boost::wave::T_LEFTBRACKET)
    {
      finishTypeList.push_back(tokens[id]);

      StatementsBuilder partial(add());

      partial.push(*it);
      ++it;
      IS_EQUAL_BREAK(it, end)

      if (partial.parseVariablesFromScopeToSemicolon(it, end, finishTypeList) == false)
      {
        return false;
      }

      partial.push(*it);
      finishTypeList.pop_back();

      IS_EQUAL_BREAK(it, end)
      ++it;

      continue;
    }

    push(*it);
    ++it;
  }

  return true;
}

}
}
