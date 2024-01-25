# README

## general information

In this repository you find prototype implementations for MADPE and TBPE as a proof of concept.

## How to use the examples

* You need to install PRISM
* For automatic extraction from logs (TBPE) to build diagrams, you need to install CMake and a C++ compiler. Then build the subdirectory `prism-log-analysis`.


* To run an example, use powershell VERSION 7 or higher to execute the *.ps1 scripts:

   * use the `MADPE_leader*` scripts for calculating the MADPE-optimal scheduler for an concrete instance of the leader election protocol examples
   * use `MADPE_coin*` scripts for MADPE examples relying on the Randomised Consensus Shared Coin Protocol
   * use `TBPE_leader*` scripts for threshold-based penalty applied on the leader election protocol examples. Here you can use the implementation inside `prism-log-analysis` to generate diagrams to be used in a LaTeX environment.

Note: For TBPE, in this implementation the values of the penalized measure come with some offset such that for any path collecting zero reward, the penalized value is also zero.