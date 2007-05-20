local t = Def.ActorFrame {
	LoadFont("Common", "normal") .. {
		Text=ProductVersion();
		OnCommand=cmd(x,SCREEN_RIGHT-20;y,SCREEN_TOP+20;horizalign,right;diffuse,0.6,0.6,0.6,1;zoom,0.5;shadowlength,2);
	};
	LoadFont("Common", "normal") .. {
		OnCommand=cmd(x,SCREEN_LEFT+20;y,SCREEN_TOP+20;horizalign,left;diffuse,0.6,0.6,0.6,1;zoom,0.5;shadowlength,2;playcommand,"Set");
		SetCommand=function(self)
			local fmt = THEME:GetString( "ScreenTitleMenu", "%d songs in %d groups, %d courses in %d groups" )
			local text = string.format( fmt, SONGMAN:GetNumSongs(), SONGMAN:GetNumSongGroups(), SONGMAN:GetNumCourses(), SONGMAN:GetNumCourseGroups() )

			if PREFSMAN:GetPreference("UseUnlockSystem") then
				text = text .. string.format(", %d unlocks", UNLOCKMAN:GetNumUnlocks() )
			end
			self:settext( text )
		end;
	};
	LoadFont("Common", "normal") .. {
		Text="Current Gametype: "..GAMESTATE:GetCurrentGame():GetName();
		OnCommand=cmd(x,SCREEN_LEFT+20;y,SCREEN_TOP+36;horizalign,left;diffuse,0.6,0.6,0.6,1;zoom,0.5;shadowlength,2);
	};
}

return t;
