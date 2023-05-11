import datetime
import math
import os
import random
import subprocess
import sys
import shutil

FuzzFactor = 30
sumatraPDF_exe = "C:/Users/vboxuser/AppData/Local/SumatraPDF/SumatraPDF.exe" 
pdf_input_dir = "C:/Users/vboxuser/Desktop/fuzzer/Fuzzer/pdfs"
pdf_output_dir = "C:/Users/vboxuser/Desktop/fuzzer/Fuzzer/pdfs-corrupt/"
corrupt_output_file = "C:/Users/vboxuser/Desktop/fuzzer/Fuzzer/output-corrupt.txt"
reg_output_file = "C:/Users/vboxuser/Desktop/fuzzer/Fuzzer/output-reg.txt"
pdf_crashed_dir = "C:/Users/vboxuser/Desktop/fuzzer/Fuzzer/crashed-pdfs/"

def clean_corrupt_folder():
    for root, subFolders, files in os.walk(pdf_output_dir):
        for f in files:
            os.unlink(os.path.join(root, f))
def clean_corrupt_txt():
    open(corrupt_output_file, 'w').close()
def clean_reg_txt():
     open(reg_output_file, 'w').close()
def run_corrupt(fuzz_type, fuzz_count):
    for i in range(fuzz_count):
        clean_corrupt_folder()
        clean_corrupt_txt()
        time_start =  str(datetime.datetime.now())
        for root, subFolders, files in os.walk(pdf_input_dir):
            for file in files:
                buf = bytearray(open(os.path.join(root,file), 'rb').read())
                numwrites = random.randrange(math.ceil((float(len(buf)) / FuzzFactor))) + 1

                for j in range(numwrites):
                    rbyte = random.randrange(256)
                    rn = random.randrange(len(buf))
                    buf[rn] = rbyte

                open(pdf_output_dir + file[:-4] + "-corrupt" + str(rbyte) + ".pdf", 'wb+').write(buf)
                if fuzz_type == 'fuzzer':
                    process = subprocess.Popen([sumatraPDF_exe, pdf_output_dir + file[:-4] + "-corrupt" + str(rbyte) + ".pdf"], errors='ignore', stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
                    crashed = process.poll()
                    if crashed:
                        open(pdf_crashed_dir + file[:-4] + "-corrupt" + str(rbyte) + ".pdf", 'wb+').write(buf)
                    else:
                        process.terminate()
        if fuzz_type == 'bench':
            result = subprocess.Popen([sumatraPDF_exe, '-bench', pdf_output_dir[:-1]], errors='ignore', stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            if result.poll():
                open(corrupt_output_file, 'w').write(result.stdout)
                break
            else:
                result.terminate()
    return

def run_reg(fuzz_type, fuzz_count):
    for i in range(fuzz_count):
        clean_reg_txt()
        if fuzz_type == "fuzzer":
            for root, subFolders, files in os.walk(pdf_input_dir):
                for file in files:
                    result = subprocess.Popen([sumatraPDF_exe, os.path.join(root,file)], errors='ignore', stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
                    if result.poll():
                        open(reg_output_file, 'w').write(result.stdout)
                    else:
                        result.terminate()
        if fuzz_type == 'bench':
            result = subprocess.Popen([sumatraPDF_exe, '-bench', pdf_input_dir], errors='ignore', stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            if result.poll():
                open(reg_output_file, 'w').write(result.stdout)
                break
            else:
                result.terminate()
    return
if __name__ == '__main__':
    fuzzer_type = sys.argv[1]
    fuzzer_pdf = sys.argv[2]
    fuzzer_count = int(sys.argv[3])
    if fuzzer_count < 0 or fuzzer_type not in ['bench', 'fuzzer']:
        print('invalid input')
    elif fuzzer_pdf == 'corrupt':
        run_corrupt(fuzzer_type, fuzzer_count)
    elif fuzzer_pdf == 'reg':
        run_reg(fuzzer_type, fuzzer_count)
    else:
        print("invalid input")
