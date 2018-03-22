//
// Created by hesham on 3/21/18.
//

#include "rexparser.h"
#include <sstream>
#include <algorithm>
#include <ctype.h>
#include "rexplib.h"
#include <iostream>

// trim from start (in place)
static inline void ltrim (std::string &s)
{
  s.erase (s.begin (), std::find_if (s.begin (), s.end (), [] (int ch)
  {
    return !std::isspace (ch);
  }));
}

// trim from end (in place)
static inline void rtrim (std::string &s)
{
  s.erase (std::find_if (s.rbegin (), s.rend (), [] (int ch)
  {
    return !std::isspace (ch);
  }).base (), s.end ());
}

// trim from both ends (in place)
static inline void trim (std::string &s)
{
  ltrim (s);
  rtrim (s);
}

// trim from both ends (copying)
static inline std::string trim_copy (std::string s)
{
  trim (s);
  return s;
}

machine rexparser::rules2nfa (const std::string rules)
{
  machine nfa ("NFA");
  std::stringstream iss (rules);

  while (iss.good ())
    {
      std::string SingleLine;
      getline (iss, SingleLine, '\n');
      process_line (trim_copy (SingleLine));
    }

  for (auto i = machines.begin (); i != machines.end (); i++)
    i->second.print_machine ();
  //Process NFA first;
  return nfa;
}

void rexparser::process_line (const std::string line)
{
  if (line.empty ())
    return;

  /*Identify line type; supported types are:
   *
   * Regular definitions are lines in the form LHS = RHS.
   * Regular expressions are lines in the form LHS: RHS.
   * Keywords are enclosed by { } in separate lines.
   * Punctuations are enclosed by [ ] in separate lines.
   */

  if ((line.find_first_of ("{") == (size_t) 0) &&
      (line.find_last_of ("}") == (line.size () - 1)))
    {
      handler_keyword (line);
    }
  else if ((line.find_first_of ("[") == (size_t) 0) &&
           (line.find_last_of ("]") == (line.size () - 1)))
    {
      handler_punctuation (line);
    }
  else if (std::find (line.begin (), line.end (), '=') != line.end ())
    {
      size_t index = line.find_first_of ("=");
      std::string machine_identifier = trim_copy (line.substr (0, index));
      machine definition (handler_regular (trim_copy (line.substr (index + 1))));
      //change_machine_identifier
      machines.insert (std::make_pair (machine_identifier, definition));
    }
  else if (std::find (line.begin (), line.end (), ':') != line.end ())
    {
      size_t index = line.find_first_of (":");
      std::string machine_identifier = trim_copy (line.substr (0, index));
      machine expression (handler_regular (trim_copy (line.substr (index + 1))));
      //change_machine_identifier
      machines.insert (std::make_pair (machine_identifier, expression));
      regex.push_back (expression);
    }
  else
    {
      //ERROR!
      //TODO:: Handle errors gracefully
    }

  return;
}
void
rexparser::push_to_stack (std::string s, std::stack<machine_stacks> &stk, std::map<std::string, machine> &_machines)
{
  machine_stacks ms;
  ms.is_operator = false;
  if (stk.empty () || stk.top ().is_operator)
    {
      machine m = get_hashed_machine (s, _machines);
      ms.identifier = s;
    }
  else
    {
      machine_stacks bottom = stk.top ();
      stk.pop ();
      machine m = machine_ops::concat (get_hashed_machine (bottom
                                                             .identifier, _machines), get_hashed_machine (s, _machines));
      ms.identifier = bottom.identifier + s;
    }
  stk.push (ms);
}
machine rexparser::handler_regular (const std::string line)
{
  std::stack<machine_stacks> st_regex;
  std::map<std::string, machine> line_machines;
  size_t index = -1;
  int matching_parentheses = 0;
  std::string holder;

  while (++index != line.size ())
    {
      char current = line.at (index);
      if (isseparator (current))
        {
          if (!holder.empty ())
            {
              push_to_stack (holder, st_regex, line_machines);
              holder = "";
            }
        }
      if (current == '(')
        {
          machine_stacks ms;
          ms.identifier = "(", ms.is_operator = true;
          st_regex.push (ms), matching_parentheses++;
        }
      else if (current == ')')
        {

          if (!matching_parentheses || st_regex.top ().is_operator)
            {
              //ERROR
              //TODO:: Handle errors gracefully
            }
          matching_parentheses--;
          std::vector<std::string> possible_result;
          std::vector<machine> or_machines;
          std::string new_machine_identifier;

          while (!st_regex.empty ())
            {
              machine_stacks _top = st_regex.top ();
              st_regex.pop ();
              if (!_top.identifier.compare ("("))
                break;
              else if (_top.is_operator && !_top.identifier.compare ("|"))
                continue;
              possible_result.push_back (_top.identifier);
            }

          for (int i = possible_result.size () - 1; i >= 0; i--)
            {
              or_machines.push_back (get_hashed_machine (possible_result.at (i), line_machines));
              new_machine_identifier += possible_result.at (i);
              if (i != 0)
                new_machine_identifier += "|";
            }

          machine_stacks ms;
          ms.identifier = new_machine_identifier, ms.is_operator = false;
          push_to_stack (new_machine_identifier, st_regex, line_machines);
          // line_machines.insert (std::make_pair (new_machine_identifier, @))  vector2machine method gets called here(@)
        }
      else if (current == '|')
        {
          machine_stacks ms;
          ms.identifier = "|", ms.is_operator = true;
          if (!st_regex.empty () && st_regex.top ().is_operator)
            {
              //ERROR
              //TODO:: Handle errors gracefully
            }
          st_regex.push (ms);
        }
      else if (current == '-')
        {
          if ((index + 1 == line.size ()) || holder.size () != 1)
            {
              //ERROR
              //TODO:: Handle errors gracefully
            }
          index++;
          std::string identifier = holder + "-" + line.at (index);
          line_machines.insert (std::make_pair (identifier, machine_ops::char_range (holder.at (0), line.at (index))));
          machine_stacks ms;
          ms.identifier = identifier, ms.is_operator = false;
          push_to_stack (identifier, st_regex, line_machines);
          holder = "";
        }
      else if (current == '*' || current == '+')
        {
          if (holder.empty ())
            {
              //ERROR
              //TODO:: Handle errors gracefully
            }
          std::string identifier = holder + current;
          if (current == '*')
            line_machines
              .insert (std::make_pair (identifier, machine_ops::star (get_hashed_machine (holder, line_machines))));
          else if (current == '+')
            line_machines
              .insert (std::make_pair (identifier, machine_ops::plus (get_hashed_machine (holder, line_machines))));
          machine_stacks ms;
          ms.identifier = identifier, ms.is_operator = false;
          push_to_stack (identifier, st_regex, line_machines);
          holder = "";
        }
      else if (current == '\\')
        {
          if (index + 1 == line.size ())
            {
              //ERROR
              //TODO:: Handle errors gracefully
            }
          index++;
          holder += line.at (index);
        }
      else
        {
          holder += current;
        }
    }

  if (!holder.empty ())
    {
      push_to_stack (holder, st_regex, line_machines);
      holder = "";
    }

  std::vector<std::string> possible_result;
  std::vector<machine> or_machines;

  // Create Final Machine
  while (!st_regex.empty ())
    {
      machine_stacks _top = st_regex.top ();
      st_regex.pop ();
      if (_top.is_operator && !_top.identifier.compare ("|"))
        continue;
      possible_result.push_back (_top.identifier);
    }

  for (int i = possible_result.size () - 1; i >= 0; i--)
    {
      or_machines.push_back (get_hashed_machine (possible_result.at (i), line_machines));
    }
  return machine_ops::oring (or_machines);
}
machine rexparser::get_hashed_machine (std::string key, std::map<std::string, machine> &_machines)
{
  if (machines.count (key))
    return machines.find (key)->second;
  else if (_machines.count (key))
    return _machines.find (key)->second;
  _machines.insert (std::make_pair (key, machine_ops::string_concat (key)));
  return _machines.find (key)->second;
}
bool rexparser::isseparator (char c)
{
  return isspace (c) || c == '|' || c == '(' || c == ')';
}

void rexparser::handler_keyword (const std::string line)
{

}
void rexparser::handler_punctuation (const std::string line)
{

}

