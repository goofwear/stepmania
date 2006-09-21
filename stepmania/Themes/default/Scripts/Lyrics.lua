function Actor:lyriccommand(side)
	self:settext( self.ctx.LyricText );

	self:stoptweening();
	self:shadowlength(0);

	local Zoom = SCREEN_WIDTH/(self:GetZoomedWidth()+1);
	if( Zoom > 1 ) then Zoom = 1 end
	self:zoomx( Zoom );

	local Color = self.ctx.LyricColor
	local Factor = 1
	if side == "Back" then
		Factor = 0.5
	end
	self:diffuse( {
		Color[1] * Factor,
		Color[2] * Factor,
		Color[3] * Factor,
		Color[4] * Factor } )

	if side == "Front" then
		self:cropright(1);
	else
		self:cropleft(0);
	end

	self:diffusealpha(0);
	self:linear(0.2);
	self:diffusealpha(0.75);
	self:linear(self.ctx.LyricDuration * 0.75);
	if side == "Front" then
		self:cropright(0);
	else
		self:cropleft(1);
	end
	self:sleep(self.ctx.LyricDuration * 0.25);
	self:linear(0.2);
	self:diffusealpha(0);
end

-- (c) 2006 Glenn Maynard
-- All rights reserved.
-- 
-- Permission is hereby granted, free of charge, to any person obtaining a
-- copy of this software and associated documentation files (the
-- "Software"), to deal in the Software without restriction, including
-- without limitation the rights to use, copy, modify, merge, publish,
-- distribute, and/or sell copies of the Software, and to permit persons to
-- whom the Software is furnished to do so, provided that the above
-- copyright notice(s) and this permission notice appear in all copies of
-- the Software and that both the above copyright notice(s) and this
-- permission notice appear in supporting documentation.
-- 
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
-- OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
-- THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
-- INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
-- OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
-- OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
-- OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
-- PERFORMANCE OF THIS SOFTWARE.

