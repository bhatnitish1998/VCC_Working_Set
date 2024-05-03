# Working set size : Estimation and Analysis

The project calculates the working set size of a VM executing different memory workloads through Simple KVM.

### Running the code

1.  **make**
2.  **./working_set <sample_interval> <sample_size> <random_percent> <workload_file> <estimation_method>**
    - **sample_interval**: The duration in seconds between each sampling.
    - **sample_size**: Number of pages used for sampling.
    - **random_percent**: The percentage of memory access in guest to be random. Ex 20% indicates that guest access 20% of its memory randomly and remaining 80% is accessed contiguously
    - **workload_file**: the path to the text file containing workload. The file should have two columns with first column indicating memory size and second column indicating the duration for which the guest should access that memory.
    - **estimation_method**: 0 for access bit based method and 1 for page invalidation method. (Methods described below).

&nbsp;

### Generating graphs.

1.  **./run.sh:** runs pre-defined set of experiments and stores the output in logs folder.
    - The experiment duration depends on the duration of different workloads (and can take more than 15 minutes).
2.  **python3 generate_graphs.py:** generates the graphs for the data in logs folder and places then in a separate graph folder.
    - needs **numpy, matplotlib** packages.

***Methods***

1.  ***Access bit method**: The 5th bit of PTE is access bit and is set whenever a page is read or written. We count the number of access bits set at certain intervals in a sample set of pages to determine the working set size.*
2.  ***Page table entry invalidation method**: In this method, the valid bit of PTE is set to 0 for certain sample pages. Whenever these pages are accessed it results in minor page fault. The number of page faults in sampling interval is used to calculate working set size.*

&nbsp;

&nbsp;
