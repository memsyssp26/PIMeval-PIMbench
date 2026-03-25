#!/usr/bin/env python3
import subprocess
import re
import csv
import os

# Configuration
BENCHMARK = "./PIMbench/vec-add/PIM/vec-add.out"
VLEN = 65536
ECC_SCHEMES = ["secded", "rs", "crc32"]
BER_LEVELS = [1e-7, 1e-6, 1e-5, 1e-4]
RESULTS_FILE = "ecc_tradeoff_results.csv"

def run_simulation(scheme, ber):
    print(f"Running: Scheme={scheme}, BER={ber}")
    cmd = [
        BENCHMARK, str(VLEN), str(VLEN),
        "--pim-ecc=1",
        f"--pim-ecc_type={scheme}",
        "--pim-ecc_granularity=64",
        f"--pim-ecc_ber={ber}"
    ]
    
    # Ensure DRAMsim3 path is set for the subprocess
    env = os.environ.copy()
    env["DRAMSIM3_PATH"] = os.path.join(os.getcwd(), "third-party/DRAMsim3")
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, env=env)
        output = result.stdout + result.stderr
        
        if ber == 1e-7 and scheme == "secded":
            print("--- DEBUG: FIRST RUN OUTPUT ---")
            print(output)
            print("--- END DEBUG ---")
        
        # Extract metrics
        stats = {
            "scheme": scheme,
            "ber": ber,
            "runtime_ms": 0.0,
            "energy_mj": 0.0,
            "pue": 0.0,
            "effective_cap_mb": 0.0,
            "corrected": 0,
            "uncorrectable": 0
        }
        
        # Parse runtime and energy from the TOTAL line
        match = re.search(r"TOTAL --------- :.* (\d+\.\d+) ms.* (\d+\.\d+) mj", output)
        if not match:
            # Try parsing from PIM Command Stats total line
            match = re.search(r"TOTAL --------- :.* (\d+\.\d+)  +(\d+\.\d+)", output)
        
        if not match:
            print(f"  Warning: Could not parse total stats for {scheme} at {ber}")
        
        if match:
            stats["runtime_ms"] = float(match.group(1))
            stats["energy_mj"] = float(match.group(2))
            
        # Parse PUE
        match = re.search(r"Prob\. of Uncorrectable Error \(PUE\) : (.*)", output)
        if match:
            stats["pue"] = float(match.group(1))
            
        # Parse capacity
        match = re.search(r"Effective System Capacity : (\d+\.\d+) MB", output)
        if match:
            stats["effective_cap_mb"] = float(match.group(1))

        # Parse error counts
        match = re.search(r"Corrected Errors : (\d+) events", output)
        if match: stats["corrected"] = int(match.group(1))
        match = re.search(r"Uncorrectable Errors : (\d+) events", output)
        if match: stats["uncorrectable"] = int(match.group(1))
            
        return stats
    except Exception as e:
        print(f"Error running simulation: {e}")
        return None

def main():
    # 1. Build project
    print("Building PIMeval...")
    env = os.environ.copy()
    env["DRAMSIM3_PATH"] = os.path.join(os.getcwd(), "third-party/DRAMsim3")
    subprocess.run(["make", "dramsim3"], check=True, capture_output=True, env=env)
    
    all_results = []
    
    # 2. Sweep
    for scheme in ECC_SCHEMES:
        for ber in BER_LEVELS:
            res = run_simulation(scheme, ber)
            if res:
                all_results.append(res)
                
    # 3. Save to CSV
    keys = all_results[0].keys()
    with open(RESULTS_FILE, 'w', newline='') as f:
        dict_writer = csv.DictWriter(f, fieldnames=keys)
        dict_writer.writeheader()
        dict_writer.writerows(all_results)
        
    print(f"\nTrade-off analysis complete. Results saved to {RESULTS_FILE}")
    print("\nSummary Table (BER vs PUE):")
    print(f"{'Scheme':<10} | {'BER':<10} | {'PUE':<10} | {'Runtime(ms)':<12} | {'Corrected':<10}")
    print("-" * 65)
    for r in all_results:
        print(f"{r['scheme']:<10} | {r['ber']:<10.1e} | {r['pue']:<10.1e} | {r['runtime_ms']:<12.4f} | {r['corrected']:<10}")

if __name__ == "__main__":
    main()
