Readme Builder
==============

About
-----
This program computes heuristically optimal schedule of given 
by action SHA1, duration and dependencies of the actions, 
which represents a direct acyclic graph (DAG).

To get the help message one can invoke the program with -h option.

    ./builder -h

or

    ./builder --help


The algorith use is Heterogenious Earliest-Finish-Time (HEFT).
    (www.scribd.com/document/261991586/2002-Topcuoglu-TPDS)

This is action planning program according to a given action dependency graph direct acyclic graph (DAG). 

Plan execution of actions according to the Heterogenious Earliest-Finish-Time HEFT algorithm.
Outputs:

    Scheduled execution time of each task SHA.

    Critical path langth and order of actions SHAs for it.


Expected input file format is each line defining action duration and its dependencies like:

    action_sha duration

or

    action_sha duration dependency_sha1 dependency_sha2 ...
    
Actions must be defined before mentioned as a dependency,
hence no circular dependency is not possible to define (and cicrular deps are not supported).
Empty lines with only whitespaces are allowed and discarded.

Schedule output file format:
    sha scheduledTime

Allowed options: :

  -h [ --help ]                  produce this help message

  -i [ --input ] arg             input file path

  -c [ --concurrency ] arg (=10) number of executors

  -p [ --critical-path ]         output critical path

  -o [ --output ] arg            output full schedule to a given path


There's an example input file `test.txt` in the root of the repository.


Build
-----
To build one need to use CMake and C++ compiler.
Clone repository, 

create out-of-source directory,

set up cmake,

build and run test:


    git clone https://github.com/nystilaps/scheduler.git
    mkdir scheduler-build
    cd scheduler-build
    cmake -DCMAKE_BUILD_TYPE=Release ../scheduler
    make -j7

Run tests:

    make test 

Run the executable like this:

    ./builder -i ../scheduler/test.txt -o '/dev/stdout' -c 10 -p

Get help on using the executable:

    ./builder -h