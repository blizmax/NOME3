#pragma once
#include "NomBaseVisitor.h"
#include "SyntaxTree.h"

namespace Nome
{

class CFileBuilder : public NomBaseVisitor
{
public:
    // The CStringBuffer is used to generate source location references
    CFileBuilder(CStringBuffer& srcStringBuffer)
        : SrcStringBuffer(srcStringBuffer)
    {
    }

    antlrcpp::Any visitFile(NomParser::FileContext* context) override;

    antlrcpp::Any visitArgClosed(NomParser::ArgClosedContext* context) override;
    antlrcpp::Any visitArgBeginCap(NomParser::ArgBeginCapContext* context) override;
    antlrcpp::Any visitArgEndCap(NomParser::ArgEndCapContext* context) override;
    antlrcpp::Any visitArgHidden(NomParser::ArgHiddenContext* context) override;
    antlrcpp::Any visitArgSurface(NomParser::ArgSurfaceContext* context) override;
    antlrcpp::Any visitArgCross(NomParser::ArgCrossContext* context) override;
    antlrcpp::Any visitArgSlices(NomParser::ArgSlicesContext* context) override;
    antlrcpp::Any visitArgOrder(NomParser::ArgOrderContext* context) override;
    antlrcpp::Any visitArgTransformTwo(NomParser::ArgTransformTwoContext* context) override;
    antlrcpp::Any visitArgTransformOne(NomParser::ArgTransformOneContext* context) override;
    antlrcpp::Any visitArgColor(NomParser::ArgColorContext* context) override;
    antlrcpp::Any visitArgControlRotate(NomParser::ArgControlRotateContext* context) override;
    antlrcpp::Any visitArgControlScale(NomParser::ArgControlScaleContext* context) override;
    antlrcpp::Any visitArgPoint(NomParser::ArgPointContext* context) override;
    antlrcpp::Any visitArgAzimuth(NomParser::ArgAzimuthContext* context) override;
    antlrcpp::Any visitArgTwist(NomParser::ArgTwistContext* context) override;
    antlrcpp::Any visitArgReverse(NomParser::ArgReverseContext* context) override;
    antlrcpp::Any visitArgMintorsion(NomParser::ArgMintorsionContext* context) override;

    antlrcpp::Any visitCmdExprListOne(NomParser::CmdExprListOneContext* context) override;
    antlrcpp::Any visitCmdIdListOne(NomParser::CmdIdListOneContext* context) override;
    antlrcpp::Any visitCmdNamedArgs(NomParser::CmdNamedArgsContext* context) override;
    antlrcpp::Any visitCmdSubCmds(NomParser::CmdSubCmdsContext* context) override;
    antlrcpp::Any visitCmdInstance(NomParser::CmdInstanceContext* context) override;
    antlrcpp::Any visitCmdSurface(NomParser::CmdSurfaceContext* context) override;
    antlrcpp::Any visitCmdArgSurface(NomParser::CmdArgSurfaceContext* context) override;
    antlrcpp::Any visitCmdBank(NomParser::CmdBankContext* context) override;
    antlrcpp::Any visitCmdDelete(NomParser::CmdDeleteContext* context) override;
    antlrcpp::Any visitCmdSubdivision(NomParser::CmdSubdivisionContext* context) override;
    antlrcpp::Any visitCmdOffset(NomParser::CmdOffsetContext* context) override;
    antlrcpp::Any visitCmdSweep(NomParser::CmdSweepContext* context) override;
    antlrcpp::Any visitSet(NomParser::SetContext* context) override;
    antlrcpp::Any visitDeleteFace(NomParser::DeleteFaceContext* context) override;

    antlrcpp::Any visitCall(NomParser::CallContext* context) override;
    antlrcpp::Any visitUnaryOp(NomParser::UnaryOpContext* context) override;
    antlrcpp::Any visitSubExpParen(NomParser::SubExpParenContext* context) override;
    antlrcpp::Any visitSubExpCurly(NomParser::SubExpCurlyContext* context) override;
    antlrcpp::Any visitBinOp(NomParser::BinOpContext* context) override;
    antlrcpp::Any visitAtom(NomParser::AtomContext* context) override
    {
        return visitChildren(context);
    }
    antlrcpp::Any visitScientific(NomParser::ScientificContext* context) override;
    antlrcpp::Any visitIdent(NomParser::IdentContext* context) override;
    antlrcpp::Any visitAtomExpr(NomParser::AtomExprContext* context) override;
    antlrcpp::Any visitIdList(NomParser::IdListContext *context) override;
    antlrcpp::Any visitVector3(NomParser::Vector3Context *context) override;

private:
    AST::CToken* ConvertToken(antlr4::Token* token);
    AST::CToken* ConvertToken(antlr4::tree::TerminalNode* token);

    CStringBuffer& SrcStringBuffer;
};

}
