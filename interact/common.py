import subprocess
import inspect

def input_case(func):
    func.position = input_case.idx
    input_case.idx += 1
    return func

input_case.idx = 0

class ProgAnalysis():
    def __init__(self, prog_path, opts=[]):
        self.prog_path = prog_path
        self.opts = opts
        self.syscalls_used = set()
        self.cases_ran = 0

    def _process_proc_output(self, proc):
        stdout_data = proc.communicate()[0]
        lines = stdout_data.splitlines()
        syscalls = lines[-1].decode()[:-1]
        self.syscalls_used = self.syscalls_used.union(set(syscalls.split(',')))
        return b'\n'.join(lines[:-1])

    def exec(self):
        proc = subprocess.Popen(["./tracer", "-m", *self.opts, '--', self.prog_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return self._process_proc_output(proc)

    def exec_with_args(self, args):
        proc = subprocess.Popen(["./tracer", "-m", *self.opts, '--', self.prog_path, *args], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return self._process_proc_output(proc)
    
    def exec_with_external(self, ext_prog, ext_args):
        proc = subprocess.Popen(["./tracer", "-m", *self.opts, '--', self.prog_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        ext_proc = subprocess.Popen([ext_prog, *ext_args])
        ext_proc.wait()
        return self._process_proc_output(proc)

    def run_all_cases(self):
        print(self.prog_path)
        methods = []
        for k in dir(self):
            method = getattr(self, k)
            if k.startswith('case_') and inspect.ismethod(method) and method.position is not None:
                methods.append((k, method))

        for k,v in sorted(methods, key=lambda el: el[1].position):
            print('-', k)
            v()
            self.cases_ran += 1

        print()

    def stats(self):
        print(self.prog_path, f'; ran {self.cases_ran} times', '; used syscalls:', self.syscalls_used)
