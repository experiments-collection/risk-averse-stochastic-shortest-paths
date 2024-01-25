
$model_size_begin = 4
$model_size_end = 5
$model_size_step = 1


$threshold_begin = 1000
$threshold_end = 100001
$threshold_step = 1000

mkdir results

for ($model_size = $model_size_begin; $model_size -lt $model_size_end; $model_size += $model_size_step){
	echo "Start with model size:   >>  $model_size  <<---------------------------------------------------"
	
	mkdir results\${model_size}
	
	for ($threshold = $threshold_begin; $threshold -lt $threshold_end; $threshold += $threshold_step){
		echo "Running for threshold:     $threshold"
		
		mkdir results\${model_size}\${threshold}

		
		cp original\leader${model_size}.nm .\results\${model_size}\${threshold}\current_model.nm
		echo "const int unfold_t = ${threshold} ;" | Add-Content .\results\${model_size}\${threshold}\current_model.nm -Encoding utf8
		Get-Content leader_reward_extension.nmext | Add-Content .\results\${model_size}\${threshold}\current_model.nm -Encoding utf8
		
		Measure-Command { prism .\results\${model_size}\${threshold}\current_model.nm .\le2.props | Out-File -FilePath .\results\${model_size}\${threshold}\log.npptxt -Encoding utf8NoBOM} | Out-File -FilePath .\results\${model_size}\${threshold}\time.npptxt -Encoding utf8NoBOM
		
		echo "time consumption:"
		
		
	}

}

