#!/usr/bin/python3

import concurrent.futures
import contextlib
import os
import shutil
import subprocess
import sys
import tempfile
import threading

if __name__ == "__main__":
	NUM_TRACES = 6
	traces = os.listdir("branch_traces")
	failed_traces = None

	if not os.path.exists("outputs"):
		os.mkdir("outputs")

	with concurrent.futures.ThreadPoolExecutor(NUM_TRACES) as executor:
		stdout_lock = threading.Lock()

		def trace_tester(trace):
			output = trace[:-4] + "_output.txt"
			actual_output = "outputs/" + output
			def run():
				subprocess.check_call(("./predictors", "-e", "branch_traces/" + trace, actual_output))
				p = subprocess.Popen(("head", "-n", str(NUM_TRACES * 2), actual_output), \
					stdout=subprocess.PIPE)
				q = subprocess.Popen(("diff", "-su", "correct_outputs/" + output, "-"), \
					universal_newlines=True, stdin=p.stdout, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

				data, _ = q.communicate()
				with stdout_lock:
					print(data)
				return q.wait()
			return executor.submit(run)

		failed_traces = \
			tuple(trace for trace, future in zip(traces, tuple(map(trace_tester, traces))) if future.result())

	if failed_traces:
		print('\nFAILED TRACES:')
		for trace in failed_traces:
			print(trace)
		if os.path.exists("outputs"):
			shutil.rmtree("outputs")
		sys.exit(1)
	else:
		print('\nALL TRACES PASSED')
