-- Overflow
print('[test] Overflow')
do
	local catches = {
		function(forge)
			forge:int(0)
		end,
		function(forge)
			forge:long(0)
		end,
		function(forge)
			forge:bool(false)
		end,
		function(forge)
			forge:float(0.0)
		end,
		function(forge)
			forge:double(0.0)
		end,
		function(forge)
			forge:string('')
		end,
		function(forge)
			forge:uri('')
		end,
		function(forge)
			forge:path('')
		end,
		function(forge)
			forge:literal('')
		end,
		function(forge)
			forge:chunk(0x1)
		end,
		function(forge)
			forge:midi(0x1)
		end,
		function(forge)
			forge:raw(1, '')
		end,
		function(forge)
			local io = Stash():read()
			forge:atom(io)
		end,
		function(forge)
			forge:urid(1, Param.sampleRate)
		end,
		function(forge)
			forge:timetag(0.1)
		end,
		function(forge)
			forge:char(0)
		end,
		function(forge)
			forge:impulse()
		end,
		function(forge)
			forge:rgba(0)
		end,
		function(forge)
			forge:time('hello')
		end,
		function(forge)
			forge:time(0.1)
		end,
		function(forge)
			forge:time(0)
		end,
		function(forge)
			forge:vector(Atom.Int):int(1):pop()
		end,
		function(forge)
			forge:vector(Atom.Int, 1, 2)
		end,
		function(forge)
			forge:vector(Atom.Int, {1, 2})
		end,
		function(forge)
			forge:vector(Atom.String)
		end,
		function(forge)
			forge:object():pop()
		end,
		function(forge)
			forge:key(1)
		end,
		function(forge)
			forge:tuple():pop()
		end,
		function(forge)
			forge:sequence():pop()
		end,
		function(forge)
			forge:message('/hello', '')
		end,
		function(forge)
			forge:bundle()
		end,
		function(forge)
			forge:get()
		end,
		function(forge)
			forge:set(Param.sampleRate):pop()
		end,
		function(forge)
			forge:put():pop()
		end,
		function(forge)
			forge:patch():pop()
		end,
		function(forge)
			forge:add():pop()
		end,
		function(forge)
			forge:remove():pop()
		end,
		function(forge)
			forge:ack(1):pop()
		end,
		function(forge)
			forge:error(1):pop()
		end,
		function(forge)
			forge:delete(1)
		end,
		function(forge)
			forge:copy(1, 2)
		end,
		function(forge)
			forge:move(1, 2)
		end,
		function(forge)
			forge:insert(1):pop()
		end,
		function(forge)
			forge:graph():pop()
		end,
		function(forge)
			forge:beginPath()
		end,
		function(forge)
			forge:closePath()
		end,
		function(forge)
			forge:arc(0, 0, 0, 0, 0)
		end,
		function(forge)
			forge:curveTo(0, 0, 0, 0, 0, 0)
		end,
		function(forge)
			forge:lineTo(0, 0)
		end,
		function(forge)
			forge:moveTo(0, 0)
		end,
		function(forge)
			forge:rectangle(0, 0, 0, 0)
		end,
		function(forge)
			forge:style(0)
		end,
		function(forge)
			forge:lineWidth(0)
		end,
		function(forge)
			forge:lineDash(0, 0)
		end,
		function(forge)
			forge:lineCap(0, 0)
		end,
		function(forge)
			forge:lineJoin(0, 0)
		end,
		function(forge)
			forge:miterLimit(0)
		end,
		function(forge)
			forge:stroke()
		end,
		function(forge)
			forge:fill()
		end,
		function(forge)
			forge:clip()
		end,
		function(forge)
			forge:save()
		end,
		function(forge)
			forge:restore()
		end,
		function(forge)
			forge:translate(0, 0)
		end,
		function(forge)
			forge:scale(0, 0)
		end,
		function(forge)
			forge:rotate(0)
		end,
		function(forge)
			forge:reset()
		end,
		function(forge)
			forge:fontSize(0)
		end,
		function(forge)
			forge:fillText('hello')
		end,
		function(forge)
			forge:read()
		end,
		function(forge)
			forge:write()
		end,
	}

	local function producer(forge)
		for i, v in ipairs(catches) do
			print(i, v)
			local stat, err = pcall(v, forge)
			assert(stat == false)
		end
	end

	local function consumer(seq)
		-- nothing
	end

	test(producer, consumer)
end
