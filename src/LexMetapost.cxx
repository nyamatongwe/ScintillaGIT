// Scintilla source code edit control

// File: LexMetapost.cxx - general context conformant metapost coloring scheme
// Author: Hans Hagen - PRAGMA ADE - Hasselt NL - www.pragma-ade.com
// Version: August 18, 2003

// Copyright: 1998-2003 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

// This lexer is derived from the one written for the texwork environment (1999++) which in
// turn is inspired on texedit (1991++) which finds its roots in wdt (1986).

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>

#include "Platform.h"

#include "PropSet.h"
#include "Accessor.h"
#include "KeyWords.h"
#include "Scintilla.h"
#include "SciLexer.h"

// Definitions in Scintilla.iface:
//
// Lexical states for SCLEX_METAPOST
//
// val SCLEX_METAPOST = 46
//
// lex METAPOST=SCLEX_METAPOST SCE_METAPOST_
//
// val SCE_METAPOST_DEFAULT = 0
// val SCE_METAPOST_SPECIAL = 1
// val SCE_METAPOST_GROUP = 2
// val SCE_METAPOST_SYMBOL = 3
// val SCE_METAPOST_COMMAND = 4
// val SCE_METAPOST_TEXT = 5

// Definitions in SciTEGlobal.properties:
//
//
// Metapost Highlighting
//
// # Default
// style.metapost.0=fore:#7F7F00
// # Special
// style.metapost.1=fore:#007F7F
// # Group
// style.metapost.2=fore:#880000
// # Symbol
// style.metapost.3=fore:#7F7F00
// # Command
// style.metapost.4=fore:#008800
// # Text
// style.metapost.5=fore:#000000

// Auxiliary functions:

static inline bool endOfLine(Accessor &styler, unsigned int i) {
	return
      (styler[i] == '\n') || ((styler[i] == '\r') && (styler.SafeGetCharAt(i + 1) != '\n')) ;
}

static inline bool isMETAPOSTcomment(char ch, char pc) {
	return
      (ch == '%') && (pc != '\\') ;
}

static inline bool isMETAPOSTone(char ch) {
	return
      (ch == '[') || (ch == ']') || (ch == '(') || (ch == ')') ||
      (ch == ':') || (ch == '=') || (ch == '<') || (ch == '>') ||
      (ch == '{') || (ch == '}') || (ch == '\'') ; // || (ch == '\"') ;
}

static inline bool isMETAPOSTtwo(char ch) {
	return
      (ch == ';') || (ch == '$') || (ch == '@') || (ch == '#');
}

static inline bool isMETAPOSTthree(char ch) {
	return
      (ch == '.') || (ch == '-') || (ch == '+') || (ch == '/') ||
      (ch == '*') || (ch == ',') || (ch == '|') || (ch == '`') ||
      (ch == '!') || (ch == '?') || (ch == '^') || (ch == '&') ||
      (ch == '%') ;
}

static inline bool isMETAPOSTidentifier(char ch) {
	return
      ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) ||
      (ch == '_') ;
}

static inline bool isMETAPOSTnumber(char ch) {
	return
      (ch >= '0') && (ch <= '9') ;
}

static inline bool isMETAPOSTstring(char ch) {
	return
      (ch == '\"') ;
}


// Coloring functions:

bool ColourMETAPOSTRange(
    unsigned int metapostMode,
    bool texMode,
    char *key,
    unsigned int endPos,
    WordList &keywords,
    Accessor &styler) {

    switch (metapostMode) {
        case 0 : // comment mode
            styler.ColourTo(endPos, SCE_METAPOST_DEFAULT) ;
            break ;
        case 1 : // special characters mode
            if (! texMode)
                { styler.ColourTo(endPos, SCE_METAPOST_SPECIAL) ; }
            else
                { styler.ColourTo(endPos, SCE_METAPOST_DEFAULT) ; }
            break ;
        case 2 : // (kind of) group mode
            if (! texMode)
                { styler.ColourTo(endPos, SCE_METAPOST_GROUP) ; }
            else
                { styler.ColourTo(endPos, SCE_METAPOST_DEFAULT) ; }
            break ;
        case 3 : // (more or less) symbol mode
            if (! texMode)
                { styler.ColourTo(endPos, SCE_METAPOST_SYMBOL) ; }
            else
                { styler.ColourTo(endPos, SCE_METAPOST_DEFAULT) ; }
            break ;
        case 4 : // command and/or keyword mode
            if (texMode) {
                if (0 == strcmp(key,"etex")) {
                    styler.ColourTo(endPos, SCE_METAPOST_COMMAND) ;
                    return false ;
                } else {
                    styler.ColourTo(endPos, SCE_METAPOST_DEFAULT) ;
                }
            } else if ((0 == strcmp(key,"btex")) || (0 == strcmp(key,"verbatimtex"))) {
                styler.ColourTo(endPos, SCE_METAPOST_COMMAND) ;
                return true ;
            } else if (keywords.InList(key)) {
                styler.ColourTo(endPos, SCE_METAPOST_COMMAND) ;
            } else {
                styler.ColourTo(endPos, SCE_METAPOST_TEXT) ;
            }
            break ;
        case 5 : // text mode
            if (! texMode)
                { styler.ColourTo(endPos, SCE_METAPOST_TEXT) ; }
            else
                { styler.ColourTo(endPos, SCE_METAPOST_DEFAULT) ; }
            break ;
        case 6 : // string mode
            styler.ColourTo(endPos, SCE_METAPOST_DEFAULT) ;
            break ;
    }
    return texMode ;
}

static void ColouriseMETAPOSTLine(
    char *lineBuffer,
    unsigned int lengthLine,
    unsigned int startPos,
    WordList &keywords,
    Accessor &styler) {

    char ch = ' ' ;
    char pc;
    unsigned int offset = 0 ;
    unsigned int mode = 5 ;
    unsigned int k = 0 ;
    char key[1024] ; // length check in calling routine
    unsigned int start = startPos-1 ;

    bool comment = true ; // does not work: (styler.GetPropertyInt("lexer.metapost.comment.process", 0) == 1) ;
    bool tex = false ;

    // we may safely assume that pc is either on the same line or a \n \r token

    // we use a cheap append to key method, ugly, but fast and ok

    while (offset < lengthLine) {

        pc = ch ;
        ch = lineBuffer[offset] ;

        if (!tex && (mode == 6)) {
            if (isMETAPOSTstring(ch)) {
                // we've run into the end of the string
                tex = ColourMETAPOSTRange(mode,tex,key,start,keywords,styler) ; k = 0 ;
                mode = 5 ;
            } else {
                // we're still in the string, comment is valid
            }
        } else if ((comment) && ((mode == 0) || (isMETAPOSTcomment(ch,pc)))) {
            tex = ColourMETAPOSTRange(mode,tex,key,start,keywords,styler) ; k = 0 ;
            mode = 0 ;
        } else if (isMETAPOSTstring(ch)) {
            if (mode != 6) { tex = ColourMETAPOSTRange(mode,tex,key,start,keywords,styler) ; k = 0 ; }
            mode = 6 ;
        } else if (isMETAPOSTone(ch)) {
            if (mode != 1) { tex = ColourMETAPOSTRange(mode,tex,key,start,keywords,styler) ; k = 0 ; }
            mode = 1 ;
        } else if (isMETAPOSTtwo(ch)) {
            if (mode != 2) { tex = ColourMETAPOSTRange(mode,tex,key,start,keywords,styler) ; k = 0 ; }
            mode = 2 ;
        } else if (isMETAPOSTthree(ch)) {
            if (mode != 3) { tex = ColourMETAPOSTRange(mode,tex,key,start,keywords,styler) ; k = 0 ; }
            mode = 3 ;
        } else if (isMETAPOSTidentifier(ch)) {
            if (mode != 4) { tex = ColourMETAPOSTRange(mode,tex,key,start,keywords,styler) ; k = 0 ; }
            mode = 4 ; key[k] = ch ; ++k ; key[k] = '\0' ;
        } else if (isMETAPOSTnumber(ch)) {
            // rather redundant since for the moment we don't handle numbers
            if (mode != 5) { tex = ColourMETAPOSTRange(mode,tex,key,start,keywords,styler) ; k = 0 ; }
            mode = 5 ;
        } else {
            if (mode != 5) { tex = ColourMETAPOSTRange(mode,tex,key,start,keywords,styler) ; k = 0 ; }
            mode = 5 ;
        }

        ++offset ;
        ++start ;

    }

    ColourMETAPOSTRange(mode,tex,key,start,keywords,styler) ;

}

// Main handler:
//
// The lexer works on a per line basis. I'm not familiar with the internals of scintilla, but
// since the lexer does not look back or forward beyond the current view, some optimization can
// be accomplished by providing just the viewport. The following code is more or less copied
// from the LexOthers.cxx file.

static void ColouriseMETAPOSTDoc(
    unsigned int startPos,
    int length,
    int /*initStyle*/,
    WordList *keywordlists[],
    Accessor &styler) {

	char lineBuffer[1024] ;
    WordList &keywords = *keywordlists[0] ;
	unsigned int linePos = 0 ;

	styler.StartAt(startPos) ;
	styler.StartSegment(startPos) ;

    for (unsigned int i = startPos; i < startPos + length; i++) {
		lineBuffer[linePos++] = styler[i] ;
		if (endOfLine(styler, i) || (linePos >= sizeof(lineBuffer) - 1)) {
			// End of line (or of line buffer) met, colourise it
			lineBuffer[linePos] = '\0' ;
			ColouriseMETAPOSTLine(lineBuffer, linePos, i-linePos+1, keywords, styler) ;
			linePos = 0 ;
		}
	}

	if (linePos > 0) {
        // Last line does not have ending characters
		ColouriseMETAPOSTLine(lineBuffer, linePos, startPos+length-linePos, keywords, styler) ;
	}

}

// Hooks info the system:

static const char * const metapostWordListDesc[] = {
	"Keywords",
	0
} ;

LexerModule lmMETAPOST(SCLEX_METAPOST, ColouriseMETAPOSTDoc, "metapost", 0, metapostWordListDesc);
