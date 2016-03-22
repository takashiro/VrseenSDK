#include "ViewManager.h"
#include "App.h"

namespace OculusCinema {

ViewManager::ViewManager() :
	m_lastViewMatrix(),
	m_views(),
	m_currentView( NULL ),
	m_nextView( NULL ),
	m_closedCurrent( false )

{
	m_lastEyeMatrix[ 0 ].SetIdentity();
	m_lastEyeMatrix[ 1 ].SetIdentity();
}

void ViewManager::addView( View * view )
{
	LOG( "AddView: %s", view->name );
    m_views.append( view );
}

void ViewManager::removeView( View * view )
{
    for( uint i = 0; i < m_views.size(); i++ )
	{
		if ( m_views[ i ] == view )
		{
            m_views.removeAt( i );
			return;
		}
	}

	// view wasn't in the array
	assert( 1 );
	LOG( "RemoveView: view not in array" );
}

void ViewManager::openView( View & view )
{
	LOG( "OpenView: %s", view.name );
	m_nextView = &view;
	m_closedCurrent = false;
}

void ViewManager::closeView()
{
	if ( m_currentView != NULL )
	{
		LOG( "CloseView: %s", m_currentView->name );
		m_currentView->OnClose();
	}
}

bool ViewManager::command(const VEvent &event)
{
	bool result = false;
    for( uint i = 0; i < m_views.size(); i++ )
	{
        result = m_views[ i ]->Command(event);
		if ( result )
		{
			break;
		}
	}

	return result;
}

bool ViewManager::onKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType )
{
	if ( ( m_currentView != NULL ) && !m_currentView->IsClosed() )
	{
		return m_currentView->OnKeyEvent( keyCode, eventType );
	}
	else
	{
		return false;
	}
}

VR4Matrixf ViewManager::drawEyeView( const int eye, const float fovDegrees )
{
	if ( m_currentView != NULL )
	{
		m_lastEyeMatrix[ eye ] = m_currentView->DrawEyeView( eye, fovDegrees );
	}

	return m_lastEyeMatrix[ eye ];
}

VR4Matrixf ViewManager::frame( const VrFrame & vrFrame )
{
	if ( ( m_nextView != NULL ) && ( m_currentView != NULL ) && !m_closedCurrent )
	{
		LOG( "OnClose: %s", m_currentView->name );
		m_currentView->OnClose();
		m_closedCurrent = true;
	}

	if ( ( m_currentView == NULL ) || ( m_currentView->IsClosed() ) )
	{
		m_currentView = m_nextView;
		m_nextView = NULL;
		m_closedCurrent = false;

		if ( m_currentView != NULL )
		{
			LOG( "OnOpen: %s", m_currentView->name );
			m_currentView->OnOpen();
		}
	}

	if ( m_currentView != NULL )
	{
		m_lastViewMatrix = m_currentView->Frame( vrFrame );
	}

	return m_lastViewMatrix;
}

} // namespace OculusCinema
