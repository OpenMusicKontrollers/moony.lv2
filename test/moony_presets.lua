function _test()
	local _state = Stash()

	if stash then
		stash(_state)
	end

	_state:read()
	if apply then
		apply(_state)
	end

	_state:write()
	if save then
		save(_state)
	end

	_state:read()
	if restore then
		restore(_state)
	end

	local n = 128
	local _control = Stash()
	local _seq1 = Stash()
	local _seq2 = Stash()
	local _seq3 = Stash()
	local _seq4 = Stash()
	local _notify = Stash()
	local _forge1 = Stash()
	local _forge2 = Stash()
	local _forge3 = Stash()
	local _forge4 = Stash()

	_control:sequence():pop()
	_control:read()

	_seq1:sequence():pop()
	_seq1:read()

	_seq2:sequence():pop()
	_seq2:read()

	_seq3:sequence():pop()
	_seq3:read()

	_seq4:sequence():pop()
	_seq4:read()

	if once then
		once(n, _control, _notify, _seq1, _forge1, _seq2, _forge2, _seq3, _forge3, _seq4, _forge4)
	end

	if run then
		run(n, _control, _notify, _seq1, _forge1, _seq2, _forge2, _seq3, _forge3, _seq4, _forge4)
	end
end

local function parse(pin)
	if not pin then return end

	local fin = io.input(pin)
	if not fin then return end

	local txt = fin:read('*all')
	fin:close()

	for id, o in string.gmatch(txt, 'moony:([^\n]*)\n%s*a pset:Preset.-moony:code%s+"""(.-)"""') do
		assert(id)
		assert(o)

		run = nil
		once = nil
		stash = nil
		apply = nil
		save = nil
		restore = nil

		print('[test] ' .. id)

		local chunk = load(o)
		assert(chunk)

		chunk()
		_test()

		collectgarbage()
	end
end

parse('presets.ttl')
