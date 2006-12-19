local stages = { }
for _, s in ipairs(Stage) do
	if GAMESTATE:IsStagePossible(s) then
		local t = LoadActor("_stage " .. s) .. {
			InitCommand = cmd(playcommand,"Update"),
			CurrentSongChangedMessageCommand=cmd(playcommand,"Update"),

			UpdateCommand=cmd(visible, GAMESTATE:GetCurrentStage() == s)
		}
		table.insert( stages, t )
	end
end

return Def.ActorFrame {
	children = stages
}
