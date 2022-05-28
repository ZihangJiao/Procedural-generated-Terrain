#include "pch.h"
#include "Input.h"


Input::Input()
{
}

Input::~Input()
{
}

void Input::Initialise(HWND window)
{
	m_keyboard = std::make_unique<DirectX::Keyboard>();
	m_mouse = std::make_unique<DirectX::Mouse>();
	m_mouse->SetWindow(window);
	m_quitApp = false;

	m_GameInput.forward		= false;
	m_GameInput.back		= false;
	m_GameInput.right		= false;
	m_GameInput.left		= false;
	m_GameInput.rotRight	= false;
	m_GameInput.rotLeft		= false;
}

void Input::Update()
{
	auto kb = m_keyboard->GetState();	//updates the basic keyboard state
	m_KeyboardTracker.Update(kb);		//updates the more feature filled state. Press / release etc. 
	auto mouse = m_mouse->GetState();   //updates the basic mouse state
	m_MouseTracker.Update(mouse);		//updates the more advanced mouse state. 

	if (kb.Escape)// check has escape been pressed.  if so, quit out. 
	{
		m_quitApp = true;
	}

	//A key
	if (kb.A)	m_GameInput.left = true;
	else		m_GameInput.left = false;
	
	//D key
	if (kb.D)	m_GameInput.right = true;
	else		m_GameInput.right = false;

	//W key
	if (kb.W)	m_GameInput.forward	 = true;
	else		m_GameInput.forward = false;

	//S key
	if (kb.S)	m_GameInput.back = true;
	else		m_GameInput.back = false;


	if (kb.LeftShift) m_GameInput.up = true;
	else           m_GameInput.up = false;

	if (kb.LeftControl) m_GameInput.down = true;
	else           m_GameInput.down = false;

	if (kb.Z) m_GameInput.upRotate = true;
	else           m_GameInput.upRotate = false;

	if (kb.X) m_GameInput.downRotate = true;
	else           m_GameInput.downRotate = false;

	//space
	if (kb.Space) m_GameInput.generate = true;
	else		m_GameInput.generate = false;

	//space
	if (kb.M) m_GameInput.smooth = true;
	else		m_GameInput.smooth = false;


	//space
	if (kb.P) m_GameInput.perlin = true;
	else		m_GameInput.perlin = false;

	//space
	if (kb.C) m_GameInput.cellular = true;
	else		m_GameInput.cellular = false;

	if (kb.V) m_GameInput.Conway = true;
	else		m_GameInput.Conway = false;

	if (kb.R) m_GameInput.initialCellAlive = true;
	else		m_GameInput.initialCellAlive = false;

	if (mouse.leftButton) {
		m_GameInput.leftButton = true;
	}
	else {
		m_GameInput.leftButton = false;
		m_GameInput.mouseX = mouse.x;
		m_GameInput.mouseY = mouse.y;
	}

}

bool Input::Quit()
{
	return m_quitApp;
}

InputCommands Input::getGameInput()
{
	return m_GameInput;
}
