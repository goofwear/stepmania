function SongSelectionScreen()
	if PlayModeName() == "Nonstop" then return "ScreenSelectCourseNonstop" end
	if PlayModeName() == "Oni" then return "ScreenSelectCourseOni" end
	if PlayModeName() == "Endless" then return "ScreenSelectCourseEndless" end
	if IsNetConnected() then ReportStyle() end
	if IsNetSMOnline() then return SMOnlineScreen() end
	if IsNetConnected() then return "ScreenNetSelectMusic" end
	return "ScreenSelectMusic"
end

function SMOnlineScreen()
	if ( not IsSMOnlineLoggedIn(1) ) and IsPlayerEnabled(1) then return "ScreenSMOnlineLogin" end
	if ( not IsSMOnlineLoggedIn(2) ) and IsPlayerEnabled(2) then return "ScreenSMOnlineLogin" end
	return "ScreenNetRoom"
end	

function SelectGameplayScreen()
	if IsExtraStage() or IsExtraStage2() then return "ScreenGameplay" end
	return "ScreenGameplay"
end

function SelectEvaluationScreen()
	if IsNetConnected() then return "ScreenNetEvaluation" end
	Mode = PlayModeName()
	if( Mode == "Regular" ) then return "ScreenEvaluationStage" end
	if( Mode == "Nonstop" ) then return "ScreenEvaluationNonstop" end
	if( Mode == "Oni" ) then return "ScreenEvaluationOni" end
	if( Mode == "Endless" ) then return "ScreenEvaluationEndless" end
	if( Mode == "Rave" ) then return "ScreenEvaluationRave" end
	if( Mode == "Battle" ) then return "ScreenEvaluationBattle" end
end

function SelectFirstOptionsScreen()
	if PlayModeName() == "Rave" then return "ScreenRaveOptions" end
	return "ScreenPlayerOptions"
end

function SelectEndingScreen()
	if GetBestFinalGrade() >= Grade("AAA") then return "ScreenMusicScroll" end
	return "ScreenCredits"
end	

function IsEventMode()
	return GetPreference( "EventMode" )
end

-- For "EvalOnFail", do:
-- function GetGameplayNextScreen() return SelectEvaluationScreen() end

function GetGameplayNextScreen()
	Trace( "GetGameplayNextScreen: " )
	local Passed = not AllFailed()
	if( Passed ) then
		Trace( "Passed" )
	else
		Trace( "Failed" )
	end
	if IsCourseMode() then Trace( "Course mode " ) end
	if IsExtraStage() then Trace( "Ex1" ) end
	if IsExtraStage2() then Trace( "Ex2" ) end
	if IsEventMode() then Trace( "Event mode" ) end
	if Passed or IsCourseMode() or
		IsExtraStage() or IsExtraStage2()
	then
		Trace( "Go to evaluation screen" )
		return SelectEvaluationScreen()
	end

	if IsEventMode() then
		Trace( "Go to song selection screen" )
		-- DeletePreparedScreens()
		return SongSelectionScreen()
	end

	Trace( "ScreenGameOver" )
	return "ScreenGameOver"
end

