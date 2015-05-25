#include "simplexmlwriter.h"
#include <util/system/oldfile.h>
#include <util/folder/dirut.h>


CSimpleXMLWriter::CSimpleXMLWriter(Stroka path, Stroka strRoot, ECharset encoding)
    : m_ActiveOutput(NULL)
    , Encoding(encoding)
    , m_bAppend(false)
{
    Open(path, strRoot, Stroka("<") + strRoot + ">", false);
}

CSimpleXMLWriter::CSimpleXMLWriter(Stroka path, Stroka strRoot, ECharset encoding,
                                   Stroka strFirstTag, bool bAppend /*= false*/)
    : m_ActiveOutput(NULL)
    , Encoding(encoding)
    , m_bAppend(false)
{
    Open(path, strRoot, strFirstTag, bAppend);
}

CSimpleXMLWriter::CSimpleXMLWriter(TOutputStream* outStream, Stroka strRoot, ECharset encoding,
                                   Stroka strFirstTag, bool bAppend /*= false*/)
    : m_ActiveOutput(outStream)
    , Encoding(encoding)
    , m_bAppend(false)
{
    Open("mapreduce", strRoot, strFirstTag, bAppend);
}

void CSimpleXMLWriter::Open(Stroka path, Stroka strRoot, Stroka strFirstTag, bool bAppend)
{
    m_strRoot = strRoot;
    m_bAppend = bAppend;
    bool bFileExists = true;

    if (path == "stdout" || path == "-" || path.empty())
        m_ActiveOutput = &Cout;
    else if (path != "mapreduce") {
        bFileExists = isexist(path.c_str());

        ui32 mode = WrOnly | Seq;
        if (bAppend)
            mode |= OpenAlways;
        else
            mode |= CreateAlways;

        TFile file(path, mode);
        if (!file.IsOpen())
            ythrow yexception() << "Can't open file " << path;

        m_outFile.Reset(new TOFStream(file));
        m_ActiveOutput = m_outFile.Get();
    }

    YASSERT(m_ActiveOutput != NULL);

    if (!bAppend || !bFileExists) {
        *m_ActiveOutput << "<?xml version='1.0' encoding='" << EncodingName() << "'?>" << Endl;
        *m_ActiveOutput << strFirstTag << Endl;
    }

    m_piDoc.Reset("1.0");
    m_piDoc.SetEncoding(Encoding);
    m_piRoot = m_piDoc.AddNewChild(strRoot.c_str());
    m_piRoot.ClearChildren();
}

CSimpleXMLWriter& CSimpleXMLWriter::operator<<(const char* s)
{
    TGuard<TMutex> lock(m_SaveStringCS);
    *m_ActiveOutput << s;
    return *this;
}

CSimpleXMLWriter& CSimpleXMLWriter::operator<<(const Stroka& s)
{
    TGuard<TMutex> lock(m_SaveStringCS);
    *m_ActiveOutput << s;
    return *this;
}

void CSimpleXMLWriter::SaveSubTreeToStream(TOutputStream& stream)
{
    TXmlNodePtrBase children = m_piRoot->children;
    for (; children.Get() != NULL; children = children->next) {
        children.SaveToStream(&stream, Encoding);
        stream << Endl;
    }
    m_piRoot.ClearChildren();
}

void CSimpleXMLWriter::SaveSubTree()
{
    SaveSubTreeToStream(*m_ActiveOutput);
}

CSimpleXMLWriter::~CSimpleXMLWriter()
{
    if (m_ActiveOutput != NULL && !m_bAppend)
        *m_ActiveOutput << "</" << m_strRoot << ">";
}
