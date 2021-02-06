function run(n, control, notify, seq, forge, ...)
	for frames, atom in seq:foreach() do
		forge:time(frames):atom(atom)
	end

	return ...
end

-- vim: set syntax=lua:
