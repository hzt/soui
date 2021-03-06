#include "souistd.h"
#include "res.mgr/SResProviderMgr.h"
#include "res.mgr/SResProvider.h"
#include "res.mgr/SSkinPool.h"
#include "res.mgr/SStylePool.h"
#include "res.mgr/SFontPool.h"
#include "res.mgr/SObjDefAttr.h"
#include "res.mgr/SNamedValue.h"

#include "helper/mybuffer.h"
#include "helper/SplitString.h"

namespace SOUI
{
    const static TCHAR KTypeFile[]      = _T("file");  //从文件加载资源时指定的类型
    const static WCHAR KNodeUidef[]     = L"uidef" ;
    const static WCHAR KNodeString[]    = L"string";
    const static WCHAR KNodeColor[]     = L"color";
    const static WCHAR KNodeSkin[]      = L"skin";
    const static WCHAR KNodeStyle[]     = L"style";
    const static WCHAR KNodeObjAttr[]   = L"objattr";
    
    pugi::xml_node GetSourceXmlNode(pugi::xml_node nodeRoot,pugi::xml_document &docInit,IResProvider *pResProvider, const wchar_t * pszName)
    {
        pugi::xml_node     nodeData = nodeRoot.child(pszName,false);
        if(nodeData)
        {
            pugi::xml_attribute attrSrc = nodeData.attribute(L"src",false);
            if(attrSrc)
            {//优先从src属性里获取数据
                SStringT strSrc = S_CW2T(attrSrc.value());
                SStringTList strList;
                if(2==ParseResID(strSrc,strList))
                {
                    CMyBuffer<char> strXml;
                    DWORD dwSize = pResProvider->GetRawBufferSize(strList[0],strList[1]);
                    
                    strXml.Allocate(dwSize);
                    pResProvider->GetRawBuffer(strList[0],strList[1],strXml,dwSize);
                    pugi::xml_parse_result result= docInit.load_buffer(strXml,strXml.size(),pugi::parse_default,pugi::encoding_utf8);
                    if(result) nodeData = docInit.child(pszName);
                }
            }
        }
        return nodeData;
    }

    class SResPackage
    {
    public:
        SResPackage(IResProvider *_pResProvider);
        ~SResPackage();
        
        void LoadUidef(LPCTSTR pszUidef);
        
        IResProvider * pResProvider;
        SSkinPool    * pSkinPool;
        SStylePool   * pStylePool;

        SNamedColor   NamedColor;
        SNamedString  NamedString;
    };

    SResPackage::SResPackage(IResProvider *_pResProvider) :pResProvider(_pResProvider),pSkinPool(NULL),pStylePool(NULL)
    {
        if(pResProvider) 
        {
            pResProvider->AddRef();
        }
    }

    SResPackage::~SResPackage()
    {
        if(pResProvider) pResProvider->Release();
        if(pSkinPool) 
        {
            SSkinPoolMgr::getSingletonPtr()->PopSkinPool(pSkinPool);
            pSkinPool->Release();
        }
        if(pStylePool)
        {
            SStylePoolMgr::getSingleton().PopStylePool(pStylePool);
            pStylePool->Release();
        }
    }

    void SResPackage::LoadUidef(LPCTSTR pszUidef)
    {
        SStringTList strUiDef;
        if(2!=ParseResID(pszUidef,strUiDef))
        {
            SLOGFMTW(_T("warning!!!! Add ResProvider Error."));
        }

        DWORD dwSize=pResProvider->GetRawBufferSize(strUiDef[0],strUiDef[1]);
        if(dwSize==0)
        {
            SLOGFMTW(_T("warning!!!! uidef was not found in the specified resprovider"));
        }else
        {
            pugi::xml_document docInit;
            CMyBuffer<char> strXml;
            strXml.Allocate(dwSize);

            pResProvider->GetRawBuffer(strUiDef[0],strUiDef[1],strXml,dwSize);

            pugi::xml_parse_result result= docInit.load_buffer(strXml,strXml.size(),pugi::parse_default,pugi::encoding_utf8);

            if(!result)
            {//load xml failed
                SLOGFMTW(_T("warning!!! load uidef as xml document failed"));
            }else
            {//init named objects
                pugi::xml_node root = docInit.child(KNodeUidef,false);
                if(!root)
                {
                    SLOGFMTW(_T("warning!!! \"uidef\" element is not the root element of uidef xml"));
                }else
                {
                    //set default font
                    pugi::xml_node xmlFont;
                    xmlFont=root.child(L"font",false);
                    if(xmlFont)
                    {
                        int nSize=xmlFont.attribute(L"size").as_int(12);
                        BYTE byCharset=(BYTE)xmlFont.attribute(L"charset").as_int(DEFAULT_CHARSET);
                        SFontPool::getSingleton().SetDefaultFont(S_CW2T(xmlFont.attribute(L"face").value()),nSize,byCharset);
                    }

                    //load named string
                    {
                        pugi::xml_document docData;
                        pugi::xml_node     nodeData = GetSourceXmlNode(root,docData,pResProvider,KNodeString);
                        if(nodeData)
                        {
                            NamedString.Init(nodeData);
                        }
                    }

                    //load named color
                    {
                        pugi::xml_document docData;
                        pugi::xml_node     nodeData = GetSourceXmlNode(root,docData,pResProvider,KNodeColor);
                        if(nodeData)
                        {
                            NamedColor.Init(nodeData);
                        }
                    }

                    //load named skin
                    {
                        pugi::xml_document docData;
                        pugi::xml_node     nodeData = GetSourceXmlNode(root,docData,pResProvider,KNodeSkin);
                        if(nodeData)
                        {
                            pSkinPool = new SSkinPool;
                            pSkinPool->LoadSkins(nodeData);
                            SSkinPoolMgr::getSingletonPtr()->PushSkinPool(pSkinPool);
                        }
                    }
                    //load named style
                    {
                        pugi::xml_document docData;
                        pugi::xml_node     nodeData = GetSourceXmlNode(root,docData,pResProvider,KNodeStyle);
                        if(nodeData)
                        {
                            pStylePool = new SStylePool;
                            pStylePool->Init(nodeData);
                            SStylePoolMgr::getSingleton().PushStylePool(pStylePool);
                        }
                    }
                    //load SWindow default attribute
                    if(SObjDefAttr::getSingleton().IsEmpty())
                    {//style只能加载一次
                        pugi::xml_document docData;
                        pugi::xml_node     nodeData = GetSourceXmlNode(root,docData,pResProvider,KNodeObjAttr);
                        if(nodeData)
                        {
                            SObjDefAttr::getSingleton().Init(nodeData);
                        }
                    }

                }
            }
        }
    }
    
    //////////////////////////////////////////////////////////////////////////

    SResProviderMgr::SResProviderMgr()
    {
    }

    SResProviderMgr::~SResProviderMgr(void)
    {
        RemoveAll();
    }

    void SResProviderMgr::RemoveAll()
    {
        SAutoLock lock(m_cs);
        SPOSITION pos=m_lstResPackage.GetHeadPosition();
        while(pos)
        {
            SResPackage *pResPackage=m_lstResPackage.GetNext(pos);
#ifdef _DEBUG//检查资源使用情况
            pResPackage->pResProvider->CheckResUsage(m_mapResUsageCount);
#endif            
            delete pResPackage;
        }
        m_lstResPackage.RemoveAll();
        
        pos = m_mapCachedCursor.GetStartPosition();
        while(pos)
        {
            CURSORMAP::CPair *pPair=m_mapCachedCursor.GetNext(pos);
            DestroyCursor(pPair->m_value);
        }
        m_mapCachedCursor.RemoveAll();
    }


    IResProvider * SResProviderMgr::GetMatchResProvider( LPCTSTR pszType,LPCTSTR pszResName )
    {
        if(!pszType) return NULL;

        SAutoLock lock(m_cs);
        SPOSITION pos=m_lstResPackage.GetTailPosition();
        while(pos)
        {
            SResPackage *pResPackage = m_lstResPackage.GetPrev(pos);
            IResProvider * pResProvider=pResPackage->pResProvider;
            if(pResProvider->HasResource(pszType,pszResName)) 
            {
#ifdef _DEBUG
                m_mapResUsageCount[SStringT().Format(_T("%s:%s"),pszType,pszResName).MakeLower()] ++;
#endif
                return pResProvider;
            }
        }
        return NULL;
    }
   
    void SResProviderMgr::AddResProvider( IResProvider * pResProvider ,LPCTSTR pszUidef)
    {
        SAutoLock lock(m_cs);
        SResPackage *pResPackage = new SResPackage(pResProvider);
        m_lstResPackage.AddTail(pResPackage);
        if(pszUidef) pResPackage->LoadUidef(pszUidef);//LoadUidef可能会通过m_lstResPackage来查询string,color等信息，需要在Add后再Load
    }

    void SResProviderMgr::RemoveResProvider( IResProvider * pResProvider )
    {
        SAutoLock lock(m_cs);
        SPOSITION pos = m_lstResPackage.GetTailPosition();
        while(pos)
        {
            SPOSITION posPrev=pos;
            SResPackage *pResPack = m_lstResPackage.GetPrev(pos);
            if(pResPack->pResProvider == pResProvider)
            {
                m_lstResPackage.RemoveAt(posPrev);
                delete pResPack;
                break;
            }
        }
    }

    LPCTSTR SResProviderMgr::SysCursorName2ID( LPCTSTR pszCursorName )
    {
        SAutoLock lock(m_cs);
        if(!_tcsicmp(pszCursorName,_T("arrow")))
            return IDC_ARROW;
        if(!_tcsicmp(pszCursorName,_T("ibeam")))
            return IDC_IBEAM;
        if(!_tcsicmp(pszCursorName,_T("wait")))
            return IDC_WAIT;
        if(!_tcsicmp(pszCursorName,_T("cross")))
            return IDC_CROSS;
        if(!_tcsicmp(pszCursorName,_T("uparrow")))
            return IDC_UPARROW;
        if(!_tcsicmp(pszCursorName,_T("size")))
            return IDC_SIZE;
        if(!_tcsicmp(pszCursorName,_T("sizenwse")))
            return IDC_SIZENWSE;
        if(!_tcsicmp(pszCursorName,_T("sizenesw")))
            return IDC_SIZENESW;
        if(!_tcsicmp(pszCursorName,_T("sizewe")))
            return IDC_SIZEWE;
        if(!_tcsicmp(pszCursorName,_T("sizens")))
            return IDC_SIZENS;
        if(!_tcsicmp(pszCursorName,_T("sizeall")))
            return IDC_SIZEALL;
        if(!_tcsicmp(pszCursorName,_T("no")))
            return IDC_NO;
        if(!_tcsicmp(pszCursorName,_T("hand")))
            return IDC_HAND;
        if(!_tcsicmp(pszCursorName,_T("help")))
            return IDC_HELP;
        return NULL;
   }

    BOOL SResProviderMgr::GetRawBuffer( LPCTSTR strType,LPCTSTR pszResName,LPVOID pBuf,size_t size )
    {
        SAutoLock lock(m_cs);
        if(IsFileType(strType))
        {
            return SResLoadFromFile::GetRawBuffer(pszResName,pBuf,size);
        }else
        {
#ifdef _DEBUG
            m_mapResUsageCount[SStringT().Format(_T("%s:%s"),strType,pszResName).MakeLower()] ++;
#endif
            IResProvider *pResProvider=GetMatchResProvider(strType,pszResName);
            if(!pResProvider) return FALSE;
            return pResProvider->GetRawBuffer(strType,pszResName,pBuf,size);
        }
    }

    size_t SResProviderMgr::GetRawBufferSize( LPCTSTR strType,LPCTSTR pszResName )
    {
        SAutoLock lock(m_cs);
        if(IsFileType(strType))
        {
            return SResLoadFromFile::GetRawBufferSize(pszResName);
        }else
        {
#ifdef _DEBUG
            m_mapResUsageCount[SStringT().Format(_T("%s:%s"),strType,pszResName).MakeLower()] ++;
#endif

            IResProvider *pResProvider=GetMatchResProvider(strType,pszResName);
            if(!pResProvider) return 0;
            return pResProvider->GetRawBufferSize(strType,pszResName);
        }
    }

    IImgX * SResProviderMgr::LoadImgX( LPCTSTR strType,LPCTSTR pszResName )
    {
        SAutoLock lock(m_cs);
        if(IsFileType(strType))
        {
            return SResLoadFromFile::LoadImgX(pszResName);
        }else
        {
#ifdef _DEBUG
            m_mapResUsageCount[SStringT().Format(_T("%s:%s"),strType,pszResName).MakeLower()] ++;
#endif

            IResProvider *pResProvider=GetMatchResProvider(strType,pszResName);
            if(!pResProvider) return NULL;
            return pResProvider->LoadImgX(strType,pszResName);
        }
    }

    IBitmap * SResProviderMgr::LoadImage( LPCTSTR pszType,LPCTSTR pszResName )
    {
        if(!pszType) return NULL;
        SAutoLock lock(m_cs);
        if(IsFileType(pszType))
        {
            return SResLoadFromFile::LoadImage(pszResName);
        }else
        {
#ifdef _DEBUG
            m_mapResUsageCount[SStringT().Format(_T("%s:%s"),pszType,pszResName).MakeLower()] ++;
#endif

            IResProvider *pResProvider=GetMatchResProvider(pszType,pszResName);
            if(!pResProvider) return NULL;
            return pResProvider->LoadImage(pszType,pszResName);
        }
    }

    HBITMAP SResProviderMgr::LoadBitmap( LPCTSTR pszResName ,BOOL bFromFile /*= FALSE*/)
    {
        SAutoLock lock(m_cs);
        if(bFromFile)
        {
            return SResLoadFromFile::LoadBitmap(pszResName);
        }else
        {
#ifdef _DEBUG
            m_mapResUsageCount[SStringT().Format(_T("bitmap:%s"),pszResName).MakeLower()] ++;
#endif

            IResProvider *pResProvider=GetMatchResProvider(KTypeBitmap,pszResName);
            if(!pResProvider) return NULL;
            return pResProvider->LoadBitmap(pszResName);
        }
    }

    HCURSOR SResProviderMgr::LoadCursor( LPCTSTR pszResName ,BOOL bFromFile /*= FALSE*/)
    {
        SAutoLock lock(m_cs);
        if(IS_INTRESOURCE(pszResName))
            return ::LoadCursor(NULL, pszResName);
        else 
        {
            LPCTSTR pszCursorID=SysCursorName2ID(pszResName);
            if(pszCursorID)
                return ::LoadCursor(NULL, pszCursorID);
        }
        const CURSORMAP::CPair * pPair  = m_mapCachedCursor.Lookup(pszResName);
        if(pPair) return pPair->m_value;
        
        HCURSOR hRet = NULL;
        if(bFromFile)
        {
            hRet = SResLoadFromFile::LoadCursor(pszResName);
        }else
        {
        
#ifdef _DEBUG
            m_mapResUsageCount[SStringT().Format(_T("cursor:%s"),pszResName).MakeLower()] ++;
#endif

            IResProvider *pResProvider=GetMatchResProvider(KTypeCursor,pszResName);
            if(pResProvider)
                hRet =pResProvider->LoadCursor(pszResName);
        }
        if(hRet)
        {
            m_mapCachedCursor[pszResName]=hRet;
        }
        return hRet;
    }

    HICON SResProviderMgr::LoadIcon( LPCTSTR pszResName,int cx/*=0*/,int cy/*=0*/ ,BOOL bFromFile /*= FALSE*/)
    {
        SAutoLock lock(m_cs);
        if(bFromFile)
        {
            return SResLoadFromFile::LoadIcon(pszResName,cx,cy);
        }else
        {
#ifdef _DEBUG
            m_mapResUsageCount[SStringT().Format(_T("icon:%s"),pszResName).MakeLower()] ++;
#endif
            IResProvider *pResProvider=GetMatchResProvider(KTypeIcon,pszResName);
            if(!pResProvider) return NULL;
            return pResProvider->LoadIcon(pszResName,cx,cy);
        }
    }

    BOOL SResProviderMgr::HasResource( LPCTSTR pszType,LPCTSTR pszResName )
    {
        SAutoLock lock(m_cs);
        if(IsFileType(pszType))
        {
            return ::GetFileAttributes(pszResName) != INVALID_FILE_ATTRIBUTES;
        }else
        {
            return NULL != GetMatchResProvider(pszType,pszResName);
        }
    }

    IBitmap * SResProviderMgr::LoadImage2(const SStringW & strImgID)
    {
        SStringT strImgID2 = S_CW2T(strImgID);
        SStringTList strLst;
        int nSegs = ParseResID(strImgID2,strLst);
        if(nSegs == 2) return LoadImage(strLst[0],strLst[1]);
        else return NULL;
    }

    HICON SResProviderMgr::LoadIcon2(const SStringW & strIconID)
    {
        SStringT strIconID2 = S_CW2T(strIconID);
        SStringTList strLst;
        int nSegs = ParseResID(strIconID2,strLst);
        if(nSegs == 2)
        {
            int cx = _ttoi(strLst[1]);
            return LoadIcon(strLst[0],cx,cx);
        }
        else 
        {
            return LoadIcon(strLst[0]);
        }
    }

    BOOL SResProviderMgr::IsFileType( LPCTSTR pszType )
    {
        if(!pszType) return FALSE;
        return _tcsicmp(pszType,KTypeFile) == 0;
    }

    COLORREF SResProviderMgr::GetColor(const SStringW & strColor)
    {//只从最后增加的资源包里查询
        SResPackage * pPackage = m_lstResPackage.GetTail();
        return pPackage->NamedColor.Get(strColor);
    }

    COLORREF SResProviderMgr::GetColor(int idx)
    {//只从第一个插入的资源包里查询
        SResPackage * pPackage = m_lstResPackage.GetHead();
        return pPackage->NamedColor.Get(idx);
    }

    SOUI::SStringW SResProviderMgr::GetString(const SStringW & strString)
    {//只从最后增加的资源包里查询
        SResPackage * pPackage = m_lstResPackage.GetTail();
        return pPackage->NamedString.Get(strString);
    }

    SOUI::SStringW SResProviderMgr::GetString(int idx)
    {//只从第一个插入的资源包里查询
        SResPackage * pPackage = m_lstResPackage.GetHead();
        return pPackage->NamedString.Get(idx);
    }
}
