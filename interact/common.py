import subprocess
import inspect

class ProgAnalysis():
    def __init__(self, prog_path, prog_args=[], opts=[]):
        self.prog_path = prog_path
        self.prog_args = prog_args
        self.opts = opts
        self.syscalls_used = set()
        self.cases_ran = 0

    def exec(self):
        proc = subprocess.Popen(["./tracer", "-m", *self.opts, self.prog_path, *self.prog_args], stdout=subprocess.PIPE)
        stdout_data = proc.communicate()[0]
        syscalls = stdout_data.splitlines()[-1].decode()[:-1]
        self.syscalls_used = self.syscalls_used.union(set(syscalls.split(',')))

    def run_all_cases(self):
        print(self.prog_path, ' '.join(self.prog_args))
        methods = []
        for k in dir(self):
            if k.startswith('case_') and inspect.ismethod(getattr(self, k)):
                methods.append((k, getattr(self, k)))

        for k,v in methods:
            print('-', k)
            v()
            self.cases_ran += 1

        print()

    def stats(self):
        print(self.prog_path, ' '.join(self.prog_args), f'; ran {self.cases_ran} times', '; used syscalls:', self.syscalls_used)
