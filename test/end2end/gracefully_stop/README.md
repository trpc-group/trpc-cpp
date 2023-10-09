### trpc-cpp gracefully stop end2end testing

Mainly, it simulates scenarios where the process receives the SIGUSR2 signal (kill -12 pid) in the server (helloworld directory) and relay (forward directory) scenarios. The goal is to check if the process can gracefully exit without crashing and without generating a core dump