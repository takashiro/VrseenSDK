#include "VArray.h"
#include "VMath.h"
#include "Input.h"
#include "View.h"

#if !defined( ViewManager_h )
#define ViewManager_h

namespace OculusCinema {

class ViewManager
{
public:
					ViewManager();

    View *			currentView() const { return m_currentView; }

    void 			addView( View * view );
    void 			removeView( View * view );

    void 			openView( View & view );
    void 			closeView();

    bool 			command( const char * msg );
    bool 			onKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType );

    VR4Matrixf 		drawEyeView( const int eye, const float fovDegrees );
    VR4Matrixf 		frame( const VrFrame & vrFrame );

private:
    VR4Matrixf		m_lastViewMatrix;
    VR4Matrixf		m_lastEyeMatrix[ 2 ];

    VArray<View *> 	m_views;

    View *			m_currentView;
    View *			m_nextView;

    bool			m_closedCurrent;
};

} // namespace OculusCinema

#endif // ViewManager_h
