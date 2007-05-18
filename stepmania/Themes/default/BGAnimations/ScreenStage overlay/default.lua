local t = Def.ActorFrame {
	LoadSongBackground() .. {
		Condition=not GAMESTATE:IsCourseMode();
		OnCommand=cmd(zoom,0;rotationz,0;linear,.5;diffusealpha,1;zoom,1;rotationz,1080);
	};
};
local OnCommand=cmd(x,SCREEN_CENTER_X;y,SCREEN_CENTER_Y+180;diffusealpha,0;linear,0.3;diffusealpha,0.75);
local function AddChild( name )
	t[#t+1] = LoadActor( name .. ".png" ) .. {
		OnCommand=OnCommand;
		Condition=GAMESTATE:GetCurrentStage() == "Stage_" .. name;
	};
end
for i = 1, 6, 1 do
	AddChild( i );
end
AddChild( "Final" );
AddChild( "Extra1" );
AddChild( "Extra2" );
AddChild( "Event" );
AddChild( "Nonstop" );
AddChild( "Oni" );
AddChild( "Endless" );
return t;
