
base = 0
sp = None
for line in file("x"):
    fields = line.split()
    if len(fields) < 2:
        print ' '.join(fields)
    elif fields[0] == "base":
        base = int(fields[1].strip("."), 16)
    elif fields[0].startswith("stack"):
        if sp == None:
            sp = int(fields[0][6:-1], 16)
            print 'sp = %x' % sp

        stack = int(fields[-1])
        fields[-1] = hex(stack)
        fields.append("  ")
        offset = stack - sp
        if stack < base:
            fields.append("<b")
        elif offset < 0:
            fields.append("b:")
            fields.append(hex(stack - base))
        else:
            fields.append("s:")
            fields.append(hex(stack - sp))

        print ' '.join(fields)
    else:
        print ' '.join(fields)


