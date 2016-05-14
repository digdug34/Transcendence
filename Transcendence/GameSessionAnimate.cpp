//	CGameSession.cpp
//
//	CGameSession class
//	Copyright (c) 2016 by Kronosaur Productions, LLC. All Rights Reserved.

#include "PreComp.h"
#include "Transcendence.h"

const CG32bitPixel BAR_COLOR = CG32bitPixel(0, 2, 10);

void CGameSession::OnAnimate (CG32bitImage &Screen, bool bTopMost)

//  OnAnimate
//
//  Paint once per frame

    {
    CSmartLock Lock(g_pUniverse->GetSem());

	bool bFailed = false;

	//	Update context

	SSystemUpdateCtx UpdateCtx;

	try
		{
		SetProgramState(psAnimating);

		//	Do the appropriate thing

		switch (g_pTrans->m_State)
			{
            case CTranscendenceWnd::gsInGame:
			case CTranscendenceWnd::gsDestroyed:
				{
				DWORD dwStartTimer;
				if (m_Settings.GetBoolean(CGameSettings::debugVideo))
					dwStartTimer = ::GetTickCount();

				//	Figure out some stats

				DWORD dwViewportFlags = 0;
				bool bBlind = false;
				bool bShowMapHUD = false;
				CShip *pShip = NULL;
				if (g_pTrans->GetPlayer())
					{
					pShip = g_pTrans->GetPlayer()->GetShip();
					bBlind = pShip->IsBlind();
					bShowMapHUD = g_pTrans->GetPlayer()->IsMapHUDActive();

					if (pShip->IsSRSEnhanced())
						dwViewportFlags |= CSystem::VWP_ENHANCED_DISPLAY;
					}

				//	If we're showing damage flash, fill the screen

				if (m_iDamageFlash > 0 && (m_iDamageFlash % 2) == 0)
					{
					Screen.Set(CG32bitPixel(128,0,0));
					if (pShip && pShip->GetSystem())
						{
						if (g_pTrans->m_bShowingMap)
							g_pUniverse->PaintObjectMap(Screen, g_pTrans->m_rcMainScreen, pShip);
						else
							g_pUniverse->PaintObject(Screen, g_pTrans->m_rcMainScreen, pShip);
						}
					}

				//	Otherwise, if we're in map mode, paint the map

				else if (g_pTrans->m_bShowingMap)
					{
					SetProgramState(psPaintingMap);
					g_pTrans->PaintMap();
					SetProgramState(psAnimating);
					}

				//	Otherwise, if we're blind, paint scramble

				else if (bBlind 
						&& (g_pTrans->m_iTick % (20 + (((g_pTrans->m_iTick / 100) * pShip->GetDestiny()) % 100))) > 15)
					g_pTrans->PaintSRSSnow();

				//	Otherwise, paint the normal SRS screen

				else
					{
					SetProgramState(psPaintingSRS);
					g_pUniverse->PaintPOV(Screen, m_rcScreen, dwViewportFlags);
					SetProgramState(psAnimating);
					}

				if (m_iDamageFlash > 0)
					m_iDamageFlash--;

				//	Paint various displays

				SetProgramState(psPaintingLRS);
				g_pTrans->PaintLRS();

				if (!g_pTrans->m_bShowingMap || bShowMapHUD)
					{
                    m_HUD.Update(g_pTrans->m_iTick);
                    m_HUD.Paint(Screen);

					SetProgramState(psPaintingDeviceDisplay);
					g_pTrans->m_DeviceDisplay.Paint(Screen);
					}

				if (g_pTrans->m_CurrentPicker == CTranscendenceWnd::pickNone)
					{
					SetProgramState(psPaintingMessageDisplay);
					g_pTrans->m_MessageDisplay.Paint(Screen);
					}

				SetProgramState(psAnimating);

				if (g_pTrans->m_CurrentMenu != CTranscendenceWnd::menuNone)
					g_pTrans->m_MenuDisplay.Paint(Screen);
				if (g_pTrans->m_CurrentPicker != CTranscendenceWnd::pickNone)
					g_pTrans->m_PickerDisplay.Paint(Screen);
				if (g_pTrans->m_bDebugConsole)
					g_pTrans->m_DebugConsole.Paint(Screen);

#ifdef DEBUG_LINE_OF_FIRE
				if (GetPlayer())
					{
					if (!GetPlayer()->GetShip()->IsLineOfFireClear(GetPlayer()->GetShip()->GetPos(),
							NULL,
							GetPlayer()->GetShip()->GetRotation()))
						g_pUniverse->DebugOutput("line of fire blocked");
					}
#endif
#ifdef DEBUG
				g_pTrans->PaintDebugLines();
#endif
				g_pTrans->m_pTC->PaintDebugInfo(Screen, m_rcScreen);

				//	Paint soundtrack info

				if (m_Settings.GetBoolean(CGameSettings::debugSoundtrack)
						&& !g_pTrans->m_bShowingMap)
					PaintSoundtrackTitles(Screen);

                //  Paint the mouse cursor, if necessary

                if (g_pTrans->GetPlayer() && g_pTrans->GetPlayer()->IsMouseAimEnabled())
                    {
                    int iMouseAimAngle;
                    int xMouse, yMouse;
                    if (g_pHI->GetMousePos(&xMouse, &yMouse))
                        {
                        int xCenter = m_rcScreen.left + RectWidth(m_rcScreen) / 2;
                        int yCenter = m_rcScreen.top + RectHeight(m_rcScreen) / 2;
                        iMouseAimAngle = ::IntVectorToPolar(xMouse - xCenter, yCenter - yMouse);
                        CPaintHelper::PaintArrow(Screen, xMouse, yMouse, iMouseAimAngle, g_pHI->GetVisuals().GetColor(colorTextHighlight));
                        }
                    else
                        iMouseAimAngle = -1;

                    g_pTrans->GetPlayer()->SetMouseAimAngle(iMouseAimAngle);
                    }

				//	Figure out how long it took to paint

				if (m_Settings.GetBoolean(CGameSettings::debugVideo))
					{
					DWORD dwNow = ::GetTickCount();
					g_pTrans->m_iPaintTime[g_pTrans->m_iFrameCount % FRAME_RATE_COUNT] = dwNow - dwStartTimer;
					dwStartTimer = dwNow;
					}

				//	Some debug information

				if (m_Settings.GetBoolean(CGameSettings::debugVideo))
					g_pTrans->PaintFrameRate();

				//	Update the screen

				if (bTopMost)
					g_pHI->GetScreenMgr().Render();

				//	Figure out how long it took to blt

				if (m_Settings.GetBoolean(CGameSettings::debugVideo))
					{
					DWORD dwNow = ::GetTickCount();
					g_pTrans->m_iBltTime[g_pTrans->m_iFrameCount % FRAME_RATE_COUNT] = dwNow - dwStartTimer;
					dwStartTimer = dwNow;
					}

				//	Update the universe

				if (!g_pTrans->m_bPaused || g_pTrans->m_bPausedStep)
					{
					SetProgramState(psUpdating);
					g_pUniverse->Update(UpdateCtx);
					if (g_pTrans->m_bAutopilot)
						{
						g_pUniverse->Update(UpdateCtx);
						g_pUniverse->Update(UpdateCtx);
						g_pUniverse->Update(UpdateCtx);
						g_pUniverse->Update(UpdateCtx);
						}
					SetProgramState(psAnimating);

					if (g_pTrans->GetPlayer())
						g_pTrans->GetPlayer()->Update(g_pTrans->m_iTick);
					if (g_pTrans->GetPlayer() && g_pTrans->GetPlayer()->GetSelectedTarget())
						m_HUD.Invalidate(hudTargeting);
					g_pTrans->m_iTick++;

					g_pTrans->m_bPausedStep = false;
					}

				g_pTrans->m_MessageDisplay.Update();

				//	Figure out how long it took to update

				if (m_Settings.GetBoolean(CGameSettings::debugVideo))
					{
					DWORD dwNow = ::GetTickCount();
					g_pTrans->m_iUpdateTime[g_pTrans->m_iFrameCount % FRAME_RATE_COUNT] = dwNow - dwStartTimer;
					dwStartTimer = dwNow;
					}

				//	Destroyed?

				if (g_pTrans->m_State == CTranscendenceWnd::gsDestroyed)
					{
					if (!g_pTrans->m_bPaused || g_pTrans->m_bPausedStep)
						{
						if (--g_pTrans->m_iCountdown == 0)
							g_pHI->HICommand(CONSTLIT("gameEndDestroyed"));
						g_pTrans->m_bPausedStep = false;
						}
					}

				break;
				}

			case CTranscendenceWnd::gsDocked:
				{
				//	Paint the screen

				g_pTrans->m_pCurrentScreen->Paint(Screen);
				g_pTrans->m_pCurrentScreen->Update();
				g_pTrans->PaintMainScreenBorder(g_pTrans->m_CurrentDock.GetVisuals().GetWindowBackgroundColor());

                //  Paint displays

                m_HUD.Update(g_pTrans->m_iTick);
                m_HUD.Paint(Screen, true);

				//	If we have even more room, paint the LRS and reactor display

				if (g_cyScreen >= 960)
					{
					SetProgramState(psPaintingLRS);
					g_pTrans->PaintLRS();
					}

				//	Debug console

				if (g_pTrans->m_bDebugConsole)
					g_pTrans->m_DebugConsole.Paint(Screen);

				//	Update the screen

				if (bTopMost)
					g_pHI->GetScreenMgr().Render();

				//	Update the universe (at 1/4 rate)

				if ((g_pTrans->m_iTick % 4) == 0)
					g_pUniverse->Update(UpdateCtx);
				g_pTrans->m_MessageDisplay.Update();
				g_pTrans->m_CurrentDock.Update(g_pTrans->m_iTick);
				g_pTrans->m_iTick++;

				//	Note: We need to invalidate the whole screen because we're
				//	flipping between two buffers and we need to make sure both
				//	buffers get painted.

				if (g_pTrans->m_pCurrentScreen)
					g_pTrans->m_pCurrentScreen->Invalidate();

				break;
				}

            case CTranscendenceWnd::gsEnteringStargate:
				{
				//	Tell the universe to paint

				g_pUniverse->PaintPOV(Screen, m_rcScreen, 0);
				g_pTrans->PaintLRS();

                m_HUD.Update(g_pTrans->m_iTick);
                m_HUD.Paint(Screen);

				g_pTrans->m_MessageDisplay.Paint(Screen);
				g_pTrans->m_DeviceDisplay.Paint(Screen);

				//	Debug information

				if (m_Settings.GetBoolean(CGameSettings::debugVideo))
					g_pTrans->PaintFrameRate();

#ifdef DEBUG
				g_pTrans->PaintDebugLines();
#endif
				g_pTrans->m_pTC->PaintDebugInfo(Screen, m_rcScreen);

				//	Update the screen

				if (bTopMost)
					g_pHI->GetScreenMgr().Render();

				//	Update the universe

				g_pUniverse->Update(UpdateCtx);
				g_pTrans->m_MessageDisplay.Update();
				g_pTrans->m_iTick++;

				if (--g_pTrans->m_iCountdown == 0)
					{
					g_pHI->HICommand(CONSTLIT("gameInsideStargate"));
					g_pTrans->m_State = CTranscendenceWnd::gsWaitingForSystem;
					}

				break;
				}

            case CTranscendenceWnd::gsWaitingForSystem:
				{
				if (g_pUniverse->GetSFXOptions().IsStargateTravelEffectEnabled())
					{
					if (g_pTrans->m_pStargateEffect == NULL)
						g_pTrans->m_pStargateEffect = new CStargateEffectPainter;

					g_pTrans->m_pStargateEffect->Paint(Screen, m_rcScreen);
					g_pTrans->m_pStargateEffect->Update();
					}
				else
					{
					Screen.Fill(m_rcScreen.left, m_rcScreen.top, RectWidth(m_rcScreen), RectHeight(m_rcScreen), BAR_COLOR);
					}

				g_pTrans->m_pTC->PaintDebugInfo(Screen, m_rcScreen);

				if (bTopMost)
					g_pHI->GetScreenMgr().Render();
				break;
				}

            case CTranscendenceWnd::gsLeavingStargate:
				{
				//	Tell the universe to paint

				g_pUniverse->PaintPOV(Screen, m_rcScreen, 0);
				g_pTrans->PaintLRS();

                m_HUD.Update(g_pTrans->m_iTick);
                m_HUD.Paint(Screen);

				g_pTrans->m_MessageDisplay.Paint(Screen);
				g_pTrans->m_DeviceDisplay.Paint(Screen);

				//	Debug information

				if (m_Settings.GetBoolean(CGameSettings::debugVideo))
					g_pTrans->PaintFrameRate();

#ifdef DEBUG
				g_pTrans->PaintDebugLines();
#endif
				g_pTrans->m_pTC->PaintDebugInfo(Screen, m_rcScreen);

				//	Update the screen

				if (bTopMost)
					g_pHI->GetScreenMgr().Render();

				//	Update the universe

				g_pUniverse->Update(UpdateCtx);
				g_pTrans->m_MessageDisplay.Update();
				g_pTrans->m_iTick++;

				if (--g_pTrans->m_iCountdown == 0)
					{
					g_pHI->HICommand(CONSTLIT("gameLeaveStargate"));
					g_pTrans->m_State = CTranscendenceWnd::gsInGame;
					}
				break;
				}

            case CTranscendenceWnd::gsEndGame:
				{
				g_pHI->HICommand(CONSTLIT("gameEndGame"));
				break;
				}
			}

		//	Flip

		if (bTopMost)
			g_pHI->GetScreenMgr().Flip();

		SetProgramState(psUnknown);
		}
	catch (...)
		{
		bFailed = true;
		}

	//	Deal with errors/crashes

	if (bFailed)
		{
		g_pHI->GetScreenMgr().StopDX();
		g_pTrans->ReportCrash();
		}
    }
