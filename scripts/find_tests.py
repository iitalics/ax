import glob, sys, os, re

test_dir = sys.argv[1]
inc_file = sys.argv[2]

with open(inc_file, 'w') as f:
    for c_file in glob.glob(os.path.join(test_dir, 'test_*.c')):
        for line in open(c_file, 'r'):
            #print(repr(line))
            m = re.match('TEST\((.*)\)', line)
            if m is not None:
                f.write('RUN_TEST({});\n'.format(m[1]))
