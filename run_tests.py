import os

My_test_folder = '/s/bach/k/under/lisaxu/CS270/Test/'
result_file = 'result.txt'
os.system('rm '+result_file)

for f in os.listdir(My_test_folder):
    if not f.endswith('.asm'):
        continue
    basename = f.split('.')[0]
    os.system('mylc3as -hex ' + My_test_folder+f)
    os.system('~cs270/lc3tools/lc3as -hex ' + My_test_folder+f)
    os.system('echo '+ f + ' --------------->> '+result_file)
    os.system('diff '+ basename+'.hex ' + My_test_folder + basename+'.hex >> '+result_file)
