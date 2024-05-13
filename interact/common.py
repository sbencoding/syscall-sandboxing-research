import subprocess
import inspect

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

    def run_all_cases(self):
        print(self.prog_path)
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
        print(self.prog_path, f'; ran {self.cases_ran} times', '; used syscalls:', self.syscalls_used)
