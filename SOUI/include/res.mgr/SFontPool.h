/**
* Copyright (C) 2014-2050 
* All rights reserved.
* 
* @file       SFontPool.h
* @brief      
* @version    v1.0      
* @author     SOUI group   
* @date       2014/08/02
* 
* Describe    SOUI字体管理模块
*/

#pragma once

#include "core/ssingletonmap.h"
#include "interface/render-i.h"
#include "unknown/obj-ref-impl.hpp"

/**
* @union      FONTSTYLE
* @brief      FONT的风格
* 
* Describe    
*/
union FONTSTYLE{
    DWORD     dwStyle;  //DWORD版本的风格
    struct
    {
        DWORD cSize:16; //字体大小，为short有符号类型
        DWORD fItalic:1;//斜体标志位
        DWORD fUnderline:1;//下划线标志位
        DWORD fBold:1;//粗体标志位
        DWORD fStrike:1;//删除线标志位
        DWORD fAbsSize:1;//字体的cSize是绝对大小还是相对于默认字体的大小,1代表cSize为绝对大小
        DWORD fReserved:3;//保留位
        DWORD byCharset:8;   //字符集
    };
    
    FONTSTYLE(DWORD _dwStyle):dwStyle(_dwStyle){}
}; 

#define FF_DEFAULTFONT FONTSTYLE(0)

/**
* @class     FontKey
* @brief      一个FONT的KEY
* 
* Describe    用于实现一个font map
*/
namespace SOUI
{
    class FontKey
    {
    public:
        FontKey(DWORD _dwStyle, 
            const SStringT & _strFaceName = SStringT(),
            const SStringT & _strPropEx = SStringT())
        {
            dwStyle=_dwStyle;
            if(_strFaceName.GetLength() <= LF_FACESIZE)
            {
                strFaceName = _strFaceName;
            }
            strPropEx = _strPropEx;
        }

        DWORD    dwStyle;
        SStringT strFaceName;
        SStringT strPropEx;
    };

    /**
    * @class     CElementTraits< FontKey >
    * @brief      FontKey的Hash及比较模板
    * 
    * Describe    用于实现一个font map
    */
    template<>
    class CElementTraits< FontKey > :
        public CElementTraitsBase<FontKey >
    {
    public:
        static ULONG Hash( INARGTYPE fontKey )
        {
            ULONG uRet=SOUI::CElementTraits<SStringT>::Hash(fontKey.strFaceName);
            uRet = (uRet<<5) + SOUI::CElementTraits<SStringT>::Hash(fontKey.strPropEx);
            uRet = (uRet<<5) +(UINT)fontKey.dwStyle+1;
            return uRet;
        }

        static bool CompareElements( INARGTYPE element1, INARGTYPE element2 )
        {
            return element1.strFaceName==element2.strFaceName
                && element1.strPropEx==element2.strPropEx
                && element1.dwStyle==element2.dwStyle;
        }

        static int CompareElementsOrdered( INARGTYPE element1, INARGTYPE element2 )
        {
            int nRet= element1.strFaceName.Compare(element2.strFaceName);
            if(nRet == 0)
                nRet = element1.strPropEx.Compare(element2.strPropEx);
            if(nRet == 0)
                nRet = element1.dwStyle-element2.dwStyle;
            return nRet;
        }
    };

    typedef IFont * IFontPtr;

    /**
    * @class      SFontPool
    * @brief      font pool
    * 
    * Describe
    */
    class SOUI_EXP SFontPool :public SSingletonMap<SFontPool,IFontPtr,FontKey>
    {
    public:
        SFontPool(IRenderFactory *pRendFactory);

        
        /**
         * GetFont
         * @brief    获得与指定的strFont对应的IFontPtr
         * @param    const SStringW & strFont --  font描述字符串
         * @return   IFontPtr -- font对象
         *
         * Describe  描述字符串格式如：face:宋体;bold:0;italic:1;underline:1;strike:1;adding:10
         */
        IFontPtr GetFont(const SStringW & strFont);

        /**
         * GetFont
         * @brief    获得与指定的font key对应的IFontPtr
         * @param    FONTSTYLE style --  字体风格
         * @param    LPCTSTR strFaceName --  字体名
         * @return   IFontPtr -- font对象
         * Describe  
         */    
        IFontPtr GetFont(FONTSTYLE style,const SStringT & strFaceName = SStringT(),const SStringT & strFontPropEx = SStringT());
        
        /**
         * SetDefaultFont
         * @brief    设置默认字体
         * @param    LPCTSTR lpszFaceName --  字体名
         * @param    LONG lSize --  字体大小
         * @param    BYTE byCharset -- 字符集
         * @return   void
         * Describe  
         */    
        void SetDefaultFont(LPCTSTR lpszFaceName, LONG lSize,BYTE byCharset=DEFAULT_CHARSET);
    protected:
        static void OnKeyRemoved(const IFontPtr & obj)
        {
            obj->Release();
        }

        IFontPtr _CreateFont(const LOGFONT &lf,const SStringT & strFontPropEx = SStringT());
        
        IFontPtr _CreateFont(FONTSTYLE style,const SStringT & strFaceName = SStringT(),const SStringT & strFontPropEx = SStringT());

        LOGFONT m_lfDefault;
        CAutoRefPtr<IRenderFactory> m_RenderFactory;
    };

}//namespace SOUI

