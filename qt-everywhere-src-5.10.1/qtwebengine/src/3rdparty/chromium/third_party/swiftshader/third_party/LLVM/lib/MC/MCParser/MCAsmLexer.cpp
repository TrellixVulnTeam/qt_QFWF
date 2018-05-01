//===-- MCAsmLexer.cpp - Abstract Asm Lexer Interface ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/Support/SourceMgr.h"

using namespace llvm;

MCAsmLexer::MCAsmLexer() : CurTok(AsmToken::Error, StringRef()), TokStart(0) {
}

MCAsmLexer::~MCAsmLexer() {
}

SMLoc MCAsmLexer::getLoc() const {
  return SMLoc::getFromPointer(TokStart);
}

SMLoc AsmToken::getLoc() const {
  return SMLoc::getFromPointer(Str.data());
}