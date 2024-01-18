#k=2
if (false){
	
$model_size_begin = 2
$model_size_end = 3
$model_size_step = 2


$threshold_begin = 1
$threshold_end = 90002
$threshold_step = 6000

mkdir results
mkdir results\consensus

for ($model_size = $model_size_begin; $model_size -lt $model_size_end; $model_size += $model_size_step){
	echo "Start with model size:   >>  $model_size  <<---------------------------------------------------"
	
	mkdir results\consensus\${model_size}
	mkdir results\consensus\${model_size}\2
	
	for ($threshold = $threshold_begin; $threshold -lt $threshold_end; $threshold += $threshold_step){
		echo "Running for threshold:     $threshold"
		
		mkdir results\consensus\${model_size}\2\${threshold}

		
		cp consensus\coin${model_size}.nm .\results\consensus\${model_size}\2\${threshold}\current_model.nm
		echo "const int unfold_t = ${threshold} ;" | Add-Content .\results\consensus\${model_size}\2\${threshold}\current_model.nm -Encoding utf8
		Get-Content leader_reward_extension.nmext | Add-Content .\results\consensus\${model_size}\2\${threshold}\current_model.nm -Encoding utf8
		
		Measure-Command { prism .\results\consensus\${model_size}\2\${threshold}\current_model.nm .\le2.props -const K=2 -maxiters 1000000 | Out-File -FilePath .\results\consensus\${model_size}\2\${threshold}\log.npptxt -Encoding utf8NoBOM} | Out-File -FilePath .\results\consensus\${model_size}\2\${threshold}\time.npptxt -Encoding utf8NoBOM
		
		
	}

}

}

$model_size_begin = 4
$model_size_end = 5
$model_size_step = 2

$threshold_begin = 1
$threshold_end = 58800
$threshold_step = 3913

mkdir results
mkdir results\consensus

for ($model_size = $model_size_begin; $model_size -lt $model_size_end; $model_size += $model_size_step){
	echo "Start with model size:   >>  $model_size  <<---------------------------------------------------"
	
	mkdir results\consensus\${model_size}
	mkdir results\consensus\${model_size}\2
	
	for ($threshold = $threshold_begin; $threshold -lt $threshold_end; $threshold += $threshold_step){
		echo "Running for threshold:     $threshold"
		
		mkdir results\consensus\${model_size}\2\${threshold}

		
		cp consensus\coin${model_size}.nm .\results\consensus\${model_size}\2\${threshold}\current_model.nm
		echo "const int unfold_t = ${threshold} ;" | Add-Content .\results\consensus\${model_size}\2\${threshold}\current_model.nm -Encoding utf8
		Get-Content leader_reward_extension.nmext | Add-Content .\results\consensus\${model_size}\2\${threshold}\current_model.nm -Encoding utf8
		
		Measure-Command { prism .\results\consensus\${model_size}\2\${threshold}\current_model.nm .\le2.props -const K=2 -maxiters 1000000 | Out-File -FilePath .\results\consensus\${model_size}\2\${threshold}\log.npptxt -Encoding utf8NoBOM} | Out-File -FilePath .\results\consensus\${model_size}\2\${threshold}\time.npptxt -Encoding utf8NoBOM
		
		
	}

}
