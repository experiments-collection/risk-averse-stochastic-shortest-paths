prism .\consensus\coin2.nm .\maximize_original_rewards.props -const K=2048 -maxiters 10000000 -exportstates current.sta -exporttrans current.tra -exportlabels current.lab -exporttransrewards current.trew | Out-File -FilePath current.log -Encoding utf8NoBOM
python build_py_gurobi_script.py

