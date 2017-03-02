function _test(produce, consume, check)
	local _from = Stash()
	local _to = Stash()

	if produce.seq then
		local _seq = _from:sequence()
		produce.seq(_seq)
		_seq:pop()
	elseif produce.tup then
		local _tup = _from:tuple()
		produce.tup(_tup)
		_tup:pop()
	end

	_from:read()

	if consume.seq then
		local _seq = _to:sequence()
		consume.seq(128, _from, _seq)
		_seq:pop()
	elseif consume.tup then
		local _tup = _to:tuple()
		consume.tup(_from, _tup)
		_tup:pop()
	end

	_to:read()

	if check then
		check(_to)
	end

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

	if once then
		local n = 128
		local _seq0 = Stash()
		local _seq1 = Stash()
		local _seq2 = Stash()
		local _seq3 = Stash()
		local _seq4 = Stash()
		local _forge0 = Stash()
		local _forge1 = Stash()
		local _forge2 = Stash()
		local _forge3 = Stash()
		local _forge4 = Stash()
		_seq0:sequence():pop()
		_seq0:read()
		_seq1:sequence():pop()
		_seq1:read()
		_seq2:sequence():pop()
		_seq2:read()
		_seq3:sequence():pop()
		_seq3:read()
		_seq4:sequence():pop()
		_seq4:read()

		once(n, _seq0, _forge0, _seq1, _forge1, _seq2, _forge2, _seq3, _forge3, _seq4, _forge4)
	end

	if run then
		local n = 128
		local _seq0 = Stash()
		local _seq1 = Stash()
		local _seq2 = Stash()
		local _seq3 = Stash()
		local _seq4 = Stash()
		local _forge0 = Stash()
		local _forge1 = Stash()
		local _forge2 = Stash()
		local _forge3 = Stash()
		local _forge4 = Stash()
		_seq0:sequence():pop()
		_seq0:read()
		_seq1:sequence():pop()
		_seq1:read()
		_seq2:sequence():pop()
		_seq2:read()
		_seq3:sequence():pop()
		_seq3:read()
		_seq4:sequence():pop()
		_seq4:read()

		run(n, _seq0, _forge0, _seq1, _forge1, _seq2, _forge2, _seq3, _forge3, _seq4, _forge4)
	end
end

local function parse(pin)
	if not pin then return end

	local fin = io.input(pin)
	if not fin then return end

	local txt = fin:read('*all')
	fin:close()

	for id, o in string.gmatch(txt, '<pre><code%s*data%-ref%s*=%s*"(.-)"%s*>(.-)</code></pre>') do
		o = string.gsub(o, '&amp;', '&')
		o = string.gsub(o, '&lt;', '<')
		o = string.gsub(o, '&gt;', '>')

		assert(id)
		assert(o)

		run = nil
		once = nil
		stash = nil
		apply = nil
		save = nil
		restore = nil

		stash_sequence = nil
		stash_tuple = nil

		apply_sequence = nil
		apply_tuple = nil

		check = nil

		print('[test] ' .. id)

		local chunk = load(o)
		assert(chunk)

		chunk()

		local _produce = {
			seq = stash_sequence,
			tup = stash_tuple
		}

		local _consume = {
			seq = apply_sequence,
			tup = apply_tuple
		}

		_test(_produce, _consume, check)

		collectgarbage()
	end
end

parse('../manual/manual.html.in')
