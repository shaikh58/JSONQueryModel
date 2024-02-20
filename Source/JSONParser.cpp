//
//  JSONParser2.cpp
//  Assignment4
//
//  Created by rick gessner on 2/16/20.
//

#include "JSONParser.h"
#include <cctype>
#include <stdexcept>
#include <cstring>

namespace ECE141 {

	using Actions = bool (JSONParser::*)(char aChar, JSONState &aState, JSONListener *);
	using parseCallback = bool(char aChar);

	// ---Basic Parsing Utilities---

	const char kColon = ':';
	const char kComma = ',';
	const char kQuote = '"';
	const char kBraceOpen = '{';
	const char kBraceClose = '}';
	const char kBracketOpen = '[';
	const char kBracketClose = ']';

	bool isWhitespace(char aChar) {
		static const char *theWS = " \t\r\n\b\xff";
		return strchr(theWS, aChar);
	}

	bool isColon(char aChar) { return kColon == aChar; }
	bool isComma(char aChar) { return kComma == aChar; }
	bool isQuote(char aChar) { return kQuote == aChar; }
	bool isNonAlphanum(char aChar) { return !isalnum(aChar) && aChar != '.'; }

	std::string readUntil(std::istream &anInput, parseCallback aCallback, bool addTerminal) {
		std::string theResult;

		while (!anInput.eof() && !(*aCallback)(anInput.peek()))
			theResult += anInput.get();

		if (addTerminal)
			if (!anInput.eof())
				theResult += anInput.get();

		return theResult;
	}

	bool skipWhile(std::istream &anInput, parseCallback aCallback) {
		while (!anInput.eof() && (*aCallback)(anInput.peek())) {
			anInput.get();
		}
		return true;
	}

	bool skipIfChar(std::istream &anInput, char aChar) {
		return (aChar == anInput.peek()) && aChar == anInput.get();
	}


	// ---JSONParser---

	JSONParser::JSONParser(std::istream &anInput) : input(anInput) {
		anInput >> std::skipws;
	}

	bool JSONParser::parse(JSONListener *aListener) {
		if (willParse(aListener)) {
			bool isValid = true;
			while (isValid) {
				skipWhile(input, isWhitespace); //skips whitespace chars in JSON file
				if (input.eof())
					break;

				const char theChar = input.get(); // returns a single character from JSON file
				isValid = parseElements(theChar, aListener);
			}

			return didParse(isValid);
		}
		return true;
	}

	bool JSONParser::didParse(bool aState) {
		return aState;
	}

	bool JSONParser::willParse(JSONListener *aListener) {
		input >> std::skipws;
		if (kBraceOpen == input.get()) {
			return handleOpenContainer(Element::object, aListener); // Open default container
		}
		return false;
	}


	bool JSONParser::handleOpenContainer(Element aType, JSONListener *aListener) {
		const JSONState theState(tempKey, aType); //tempKey data member set from parseElements - if object, tempKey is the key
		tempKey = ""; // by default, set the tempKey to "" to indicate string unless overriden in parseElements for object
		states.push(theState); // top state represents currently open container - use to indicate to Model which container to add to
		return (!aListener) || aListener->openContainer(theState.key, aType);
	}

	bool JSONParser::handleCloseContainer(Element aType, JSONListener *aListener) {
		tempKey = "";
		const std::string theKey(states.top().key);
		if (!states.empty())
			states.pop();

		return (!aListener) || aListener->closeContainer(theKey, aType);
	}


	Element determineType(char aChar) {
		const char *kConstantChars = "01234567890tfn"; // to parse numbers, true, false, null values
		switch (aChar) {
			case kQuote: // if char is a quote, the type is quoted - use this to call std::variant with string
				return Element::quoted;
			case kBraceOpen: // for open brace, we know it must be an object
				return Element::object;
			case kBracketOpen:
				return Element::array;
			case kBraceClose:
			case kBracketClose:
				return Element::closing;
			default: // if encounter neither an open/close bracket/curly brace, or quote,
				return strchr(kConstantChars, aChar) ? Element::constant : Element::unknown;
		}
	}

	// Parse all possible elements - works on a single character from JSON file
	bool JSONParser::parseElements(char aChar, JSONListener *aListener) {
		bool theResult = true;

		const Element theType = determineType(aChar);
		const JSONState &theTop = states.top(); // states elements come from handleOpenContainer method
		std::string theValue;

		switch (theType) {
			case Element::object:
			case Element::array:
				theResult = handleOpenContainer(theType, aListener);
				break;

			case Element::closing:
				theResult = handleCloseContainer(theTop.type, aListener);
				skipWhile(input, isWhitespace);
				skipIfChar(input, kComma);
				break;

			case Element::quoted: // string element - if encounter a quote, read till the next quote
				theValue = readUntil(input, isQuote, false);
				skipIfChar(input, kQuote);
				skipWhile(input, isWhitespace);
				skipIfChar(input, kComma);

				if (Element::object == theTop.type) { // if an open curly brace is the next thing after the colon and ws, it's an object
					if (kColon == input.peek()) {
						input.get(); // Skip the colon
						tempKey = theValue; // here theValue represents the key since next element is a colon (only occurs for objects)
					}
					else { // this calls addKeyValuePair with the key of the next map to be created
						theResult = (!aListener) || aListener->addKeyValuePair(tempKey, theValue, theType);
					}
				}
				else
					theResult = (!aListener) || aListener->addItem(theValue, theType);

				break;

			case Element::constant: // non-string input
				theValue = readUntil(input, isNonAlphanum, false);
				theValue.insert(0, 1, aChar);
				skipWhile(input, isWhitespace);
				skipIfChar(input, kComma);

				if (Element::object == theTop.type)
					theResult = (!aListener) || aListener->addKeyValuePair(tempKey, theValue, theType);
				else
					theResult = (!aListener) || aListener->addItem(theValue, theType);

				break;

			default:
				break;
		}

		skipWhile(input, isWhitespace);
		return theResult;
	}

} // namespace ECE141
