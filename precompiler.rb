#!/usr/bin/ruby

def die(msg)
    $stderr.puts(msg)
    exit 1
end

def process_line(line)
    line = line.dup

    result = [nil, nil]

    result[0] = /^(.*)@@/.match(line)[1]

    line.sub!(/^.*@@/, '')

    directive = line.match(/^\w*/)
    return nil unless directive
    result << directive[0]

    line.sub!(/^\w*\s*/, '')
    return nil unless line[0] == '('

    while line[0] != ')'
        line = line[1..-1]

        levels = { parens: 0, brackets: 0, braces: 0 }
        in_dq = false
        in_sq = false
        arg = ''

        while true
            if in_dq
                if line[0] == '"'
                    in_dq = false
                end
            elsif in_sq
                if line[0] == "'"
                    in_sq = false
                end
            else
                case line[0]
                when '"'
                    in_dq = true
                when "'"
                    in_sq = true
                when '('
                    levels[:parens] += 1
                when ')'
                    levels[:parens] -= 1
                    break if levels[:parens] < 0
                when '['
                    levels[:brackets] += 1
                when ']'
                    levels[:brackets] -= 1
                when '{'
                    levels[:braces] += 1
                when '}'
                    levels[:braces] -= 1
                when ','
                    break if !levels.find { |e| e[1] != 0 } && !in_sq && !in_dq
                end
            end

            arg += line[0]
            line = line[1..-1]
        end

        result << eval(arg.strip)
    end

    result[1] = line[1..-1]

    return result
end

def array_replace(a, p, r)
    # FIXME: Improve performance
    i = 0
    while i <= a.length - p.length
        j = 0
        while j < p.length
            break if a[i + j] != p[j]
            j += 1
        end

        if j == p.length
            j = 0
            while j < p.length
                if r
                    a[i + j] = "(uint8_t)(((#{r}) >> #{j * 8}) & 0xff)"
                else
                    # TODO: This is mostly convenience for asm_with_offset()...
                    #       But is there a better general way?
                    a[i + j] = '0x00'
                end
                j += 1
            end
            return i
        end

        i += 1
    end

    return nil
end

def do_process_asmr(get_offset, *args)
    input = args.shift

    IO.write('/tmp/.precompiler.asm', "use#{$bitness}\n" + input)
    system('fasm /tmp/.precompiler.{asm,bin} &> /tmp/fasm-log') || die("FASM failed\n" + IO.read('/tmp/fasm-log'))
    array = IO.read('/tmp/.precompiler.bin').unpack('C*')
    system('rm -f /tmp/.precompiler.{asm,bin} /tmp/fasm-log')

    while !args.empty?
        pattern = args.shift
        replacement = args.shift

        offset = array_replace(array, pattern, replacement)
        if !offset
            die("Did not find #{pattern.inspect} in #{array.inspect}")
        end
        if get_offset
            return offset
        end
    end

    return array.map { |b| b.kind_of?(Integer) ? ('0x%02x' % b) : b } * ', '
end

def process_asmr(*args)
    do_process_asmr(false, *args)
end

def process_asm(*args)
    '(uint8_t[]){' + process_asmr(*args) + '}'
end

def process_asmi(*args)
    '{' + process_asmr(*args) + '}'
end

def process_asm_offset(*args)
    if args.length != 2
        die("process_asm_offset() takes exactly two arguments")
    end
    do_process_asmr(true, *args).to_s
end



if ARGV.size != 3
    die("Expected exactly three arguments (<bitness> <input> <output>)")
end

$bitness = ARGV[0]
in_fn = ARGV[1]
out_fn = ARGV[2]

out = File.open(out_fn, 'w')
IO.readlines(in_fn).each do |line|
    while line.include?('@@')
        match = process_line(line)
        if match
            result = nil

            prefix = match.shift
            postfix = match.shift
            directive = match.shift

            case directive
            when 'asm' # Full-blown: (uint8_t[]){ 0x2a, 0x66 }
                result = process_asm(*match)

            when 'asmr' # Just the raw byte list: 0x2a, 0x66
                result = process_asmr(*match)

            when 'asmi' # Initializer: { 0x2a, 0x66 }
                result = process_asmi(*match)

            when 'asm_offset'
                result = process_asm_offset(*match)

            when 'asm_with_offset'
                # TODO: Optimize
                result = process_asm_offset(*(match.dup)) + ', ' + process_asm(*match)

            when 'asmi_with_offset'
                # TODO: Optimize
                result = process_asm_offset(*(match.dup)) + ', ' + process_asmi(*match)

            else
                die("Unknown precompiler directive #{directive}")
            end

            line = prefix + result + postfix
        end
    end

    out.write(line)
end
