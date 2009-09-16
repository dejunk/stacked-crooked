#include "NativeComponent.h"
#include "Defaults.h"
#include "Element.h"
#include "Layout.h"
#include "ErrorHandler/ErrorStack.h"
#include <boost/lexical_cast.hpp>
#include <string>


using namespace CppToys;


namespace XULWin
{
    int CommandID::sID = 101; // start command IDs at 101 to avoid conflicts with Windows predefined values
    
    NativeComponent::Components NativeComponent::sComponents;

    NativeComponent::NativeComponent(NativeComponentPtr inParent, CommandID inCommandID, LPCWSTR inClassName, DWORD inExStyle, DWORD inStyle) :
        mParent(inParent),
        mModuleHandle(::GetModuleHandle(0)), // TODO: Fix this hacky thingy!
        mCommandID(inCommandID)
    {
        NativeComponentPtr parent = mParent.lock();
        if (!parent && (inStyle & WS_CHILD))
        {
            throw std::runtime_error("Invalid parent");
        }

        int x = 0, y = 0;
        int w = Defaults::windowWidth();
        int h = Defaults::windowHeight();

        if (parent)
        {
            RECT rc;
            ::GetClientRect(parent->handle(), &rc);
            x = 0;
            y = 0;
            w = rc.right - rc.left;
            h = rc.bottom - rc.top;
        }
        else
        {
            x = (GetSystemMetrics(SM_CXSCREEN) - w)/2;
            y = (GetSystemMetrics(SM_CYSCREEN) - h)/2;
        }

        
        mHandle = ::CreateWindowEx
        (
            inExStyle, 
            inClassName,
            L"",            // title attribute can be set later
            inStyle,
            x, y, w, h,
            parent ? parent->handle() : 0,
            (HMENU)inCommandID.intValue(),
            mModuleHandle,
            0
        );

        int a = 0;
        int b = 0;
        sComponents.insert(std::make_pair(mHandle, this));
        a++;
        b++;
    }


    NativeComponent::~NativeComponent()
    {
        Components::iterator it = sComponents.find(mHandle);
        bool found = it != sComponents.end();
        assert(found);
        if (found)
        {
            sComponents.erase(it);
        }

        ::DestroyWindow(mHandle);
    }
    

    void NativeComponent::init()
    {
    }


    void NativeComponent::applyAttributes()
    {
        AttributesMapping & attr = owningElement()->Attributes;
        AttributesMapping::iterator it = attr.begin(), end = attr.end();
        for (; it != end; ++it)
        {
            applyAttribute(it->first, it->second);
        }
    }
    
    
    void NativeComponent::setOwningElement(Element * inElement)
    {
        mElement = inElement;
    }

    
    Element * NativeComponent::owningElement() const
    {
        return mElement;
    }


    NativeComponentPtr NativeComponent::parent() const
    {
        NativeComponentPtr result;
        if (NativeComponentPtr parent = mParent.lock())
        {
            result = parent;
        }
        return result;
    }


    HWND NativeComponent::handle() const
    {
        return mHandle;
    }
    

    void NativeComponent::rebuildLayout()
    {
        rebuildChildLayouts();
    }

    
    void NativeComponent::rebuildChildLayouts()
    {
        Children::iterator it = mElement->children().begin(), end = mElement->children().end();
        for (; it != end; ++it)
        {
            (*it)->nativeComponent()->rebuildLayout();
        }
    }

    
    LRESULT CALLBACK NativeComponent::MessageHandler(HWND hWnd, UINT inMessage, WPARAM wParam, LPARAM lParam)
    {
        Components::iterator it = sComponents.find(hWnd);
        if (it != sComponents.end())
        {
            return it->second->handleMessage(inMessage, wParam, lParam);
        }
        return ::DefWindowProc(hWnd, inMessage, wParam, lParam);
    }


    LRESULT NativeComponent::handleMessage(UINT inMessage, WPARAM wParam, LPARAM lParam)
    {
        return ::DefWindowProc(mHandle, inMessage, wParam, lParam);
    }

    void NativeWindow::Register(HMODULE inModuleHandle)
    {
        WNDCLASSEX wndClass;
        wndClass.cbSize = sizeof(wndClass);
        wndClass.style = 0;
        wndClass.lpfnWndProc = &NativeComponent::MessageHandler;
        wndClass.cbClsExtra = 0;
        wndClass.cbWndExtra = 0;
        wndClass.hInstance = inModuleHandle;
        wndClass.hIcon = 0;
        wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndClass.hbrBackground = 0; // covered by content pane so no color needed (reduces flicker)
        wndClass.lpszMenuName = NULL;
        wndClass.lpszClassName = TEXT("XULWin::Window");
        wndClass.hIconSm = 0;
        if (! RegisterClassEx(&wndClass))
        {
            throw std::runtime_error(std::string("Could not register Windows class."));
        }
    }


    void NativeWindow::showModal()
    {        
        ::ShowWindow(handle(), SW_SHOW);

        MSG message;
        while (GetMessage(&message, NULL, 0, 0))
        {
            HWND hActive = GetActiveWindow();
            if (! IsDialogMessage(hActive, &message))
            {
                TranslateMessage(&message);
                DispatchMessage(&message);
            }
        }
    }

    
    void NativeWindow::applyAttribute(const std::string & inName, const std::string & inValue)
    {
        if (inName == "width")
        {
            try
            {
                int w = boost::lexical_cast<int>(inValue);
                RECT rw;
                ::GetWindowRect(handle(), &rw);
                
                int oldWidth = rw.right - rw.left;
                int x = rw.left - (w - oldWidth)/2;
                if (w < oldWidth)
                {
                    x = rw.left + (w - oldWidth)/2;
                }
                ::MoveWindow(handle(), x, rw.top, w, rw.bottom - rw.top, FALSE);
                ::InvalidateRect(handle(), 0, FALSE);
            }
            catch (boost::bad_lexical_cast & e)
            {
                ThrowError(std::string("Failed to apply 'width' attribute to Window. Reason: ") + e.what());
                return;
            }
        }
    }


    LRESULT NativeWindow::handleMessage(UINT inMessage, WPARAM wParam, LPARAM lParam)
    {
        switch(inMessage)
        {
            case WM_SIZE:
            {
                Children::iterator it = owningElement()->children().begin();
                if (it != owningElement()->children().end())
                {
                    RECT rc;
                    ::GetClientRect(handle(), &rc);
                    HWND childHandle = (*it)->nativeComponent()->handle();
                    ::MoveWindow(childHandle, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, FALSE);
                    ::InvalidateRect(childHandle, 0, FALSE);
                    rebuildChildLayouts();
                }
                ::InvalidateRect(handle(), 0, FALSE);
                ::UpdateWindow(handle());
                break;
            }
            case WM_CLOSE:
            {
                PostQuitMessage(0);
                break;
            }
        }
        return ::DefWindowProc(handle(), inMessage, wParam, lParam);
    }


    NativeLabel::NativeLabel(NativeComponentPtr inParent) :
        NativeControl(inParent,
                      TEXT("STATIC"),
                      0, // exStyle
                      0)
    {
    }
        
        
    void NativeLabel::applyAttribute(const std::string & inName, const std::string & inValue)
    {
        if (inName == "value")
        {
            ::SetWindowTextA(handle(), inValue.c_str());
        }
    }
    
    
    void NativeHBox::rebuildLayout()
    {
        if (NativeComponentPtr parent = mParent.lock())
        {
            RECT rc;
            ::GetClientRect(handle(), &rc);

            LinearLayoutManager layoutManager(HORIZONTAL);
            
            //
            // Obtain the flex values
            //
            std::vector<int> flexValues;
            for (size_t idx = 0; idx != mElement->children().size(); ++idx)
            {
                std::string flex = mElement->children()[idx]->Attributes["flex"];

                int flexValue = Defaults::Attributes::flex();
                try
                {
                    flexValue = boost::lexical_cast<int>(flex);
                }
                catch (const boost::bad_lexical_cast & )
                {
                    ThrowError("Lexical cast failed for value: " + flex + ".");
                    // continue program flow
                }
                flexValues.push_back(flexValue);
            }
            
            //
            // Use the flex values to obtain the child rectangles
            //
            std::vector<Rect> childRects;
            layoutManager.getRects(
                Rect(rc.left,
                     rc.top,
                     rc.right-rc.left,
                     rc.bottom-rc.top),
                flexValues,
                childRects);

            //
            // Apply the new child rectangles
            //
            for (size_t idx = 0; idx != mElement->children().size(); ++idx)
            {
                Rect & rect = childRects[idx];
                HWND childHandle = mElement->children()[idx]->nativeComponent()->handle();
                ::MoveWindow(childHandle, rect.x(), rect.y(), rect.width(), rect.height(), FALSE);
                ::InvalidateRect(childHandle, 0, FALSE);
            }
        }
        rebuildChildLayouts();
    }


    LRESULT NativeHBox::handleMessage(UINT inMessage, WPARAM wParam, LPARAM lParam)
    {
        return ::DefWindowProc(handle(), inMessage, wParam, lParam);
    }
    
    
    void NativeVBox::rebuildLayout()
    {
        if (NativeComponentPtr parent = mParent.lock())
        {
            RECT rc;
            ::GetClientRect(handle(), &rc);

            LinearLayoutManager layoutManager(VERTICAL);
            
            //
            // Obtain the flex values
            //
            std::vector<int> flexValues;
            for (size_t idx = 0; idx != mElement->children().size(); ++idx)
            {
                std::string flex = mElement->children()[idx]->Attributes["flex"];

                int flexValue = Defaults::Attributes::flex();
                try
                {
                    flexValue = boost::lexical_cast<int>(flex);
                }
                catch (const boost::bad_lexical_cast & )
                {
                    ThrowError("Lexical cast failed for value: " + flex + ".");
                    // continue program flow
                }
                flexValues.push_back(flexValue);
            }
            
            //
            // Use the flex values to obtain the child rectangles
            //
            std::vector<Rect> childRects;
            layoutManager.getRects(
                Rect(rc.left,
                     rc.top,
                     rc.right-rc.left,
                     rc.bottom-rc.top),
                flexValues,
                childRects);

            //
            // Apply the new child rectangles
            //
            for (size_t idx = 0; idx != mElement->children().size(); ++idx)
            {
                Rect & rect = childRects[idx];
                HWND childHandle = mElement->children()[idx]->nativeComponent()->handle();
                ::MoveWindow(childHandle, rect.x(), rect.y(), rect.width(), rect.height(), FALSE);
                ::InvalidateRect(childHandle, 0, FALSE);
            }
        }
        rebuildChildLayouts();
    }


    LRESULT NativeVBox::handleMessage(UINT inMessage, WPARAM wParam, LPARAM lParam)
    {
        return ::DefWindowProc(handle(), inMessage, wParam, lParam);
    }


} // namespace XULWin
