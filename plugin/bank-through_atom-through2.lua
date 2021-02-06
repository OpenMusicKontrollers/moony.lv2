function run(n, control, notify, seq1, forge1, seq2, forge2)
	for frames, atom in seq1:foreach() do
		forge1:time(frames):atom(atom)
	end

	for frames, atom in seq2:foreach() do
		forge2:time(frames):atom(atom)
	end
end

-- vim: set syntax=lua:
