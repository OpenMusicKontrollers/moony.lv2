function run(n, control, notify, seq1, forge1, seq2, forge2, seq3, forge3, seq4, forge4)
	for frames, atom in seq1:foreach() do
		forge1:time(frames):atom(atom)
	end

	for frames, atom in seq2:foreach() do
		forge2:time(frames):atom(atom)
	end

	for frames, atom in seq3:foreach() do
		forge3:time(frames):atom(atom)
	end

	for frames, atom in seq4:foreach() do
		forge4:time(frames):atom(atom)
	end
end

-- vim: set syntax=lua:
