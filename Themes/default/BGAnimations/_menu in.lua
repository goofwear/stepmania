return Def.ActorFrame {
	LoadActor( THEME:GetPathS("", "_swoosh normal") ) .. {
		--StartTransitioningCommand=cmd(play);
	};
	LoadActor("_moveon") .. {
		OnCommand=cmd(x,SCREEN_CENTER_X;y,SCREEN_CENTER_Y;zoom,.8;diffusealpha,1;linear,0.2;diffusealpha,0);
	};
};
