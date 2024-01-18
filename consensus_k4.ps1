#k=4

$var_k = 4

$model_size_begin = 2
$model_size_end = 10
$model_size_step = 2


$threshold_begin = 1
$threshold_end = 109000
$threshold_step = 7200

mkdir results
mkdir results\consensus

for ($model_size = $model_size_begin; $model_size -lt $model_size_end; $model_size += $model_size_step){
	echo "Start with model size:   >>  $model_size  <<---------------------------------------------------"
	
	mkdir results\consensus\${model_size}
	mkdir results\consensus\${model_size}\${var_k}
	
	for ($threshold = $threshold_begin; $threshold -lt $threshold_end; $threshold += $threshold_step){
		echo "Running for threshold:     $threshold"
		
		mkdir results\consensus\${model_size}\${var_k}\${threshold}

		
		cp consensus\coin${model_size}.nm .\results\consensus\${model_size}\${var_k}\${threshold}\current_model.nm
		echo "const int unfold_t = ${threshold} ;" | Add-Content .\results\consensus\${model_size}\${var_k}\${threshold}\current_model.nm -Encoding utf8
		Get-Content leader_reward_extension.nmext | Add-Content .\results\consensus\${model_size}\${var_k}\${threshold}\current_model.nm -Encoding utf8
		
		Measure-Command { prism .\results\consensus\${model_size}\${var_k}\${threshold}\current_model.nm .\le2.props -const K=${var_k} -maxiters 1000000 | Out-File -FilePath .\results\consensus\${model_size}\${var_k}\${threshold}\log.npptxt -Encoding utf8NoBOM} | Out-File -FilePath .\results\consensus\${model_size}\${var_k}\${threshold}\time.npptxt -Encoding utf8NoBOM
		
		
	}

}

