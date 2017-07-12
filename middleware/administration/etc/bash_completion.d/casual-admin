# Do not load, if casual-admin is not in $PATH
which casual-admin &> /dev/null || return 0

__no_colors() {
	sed -r "s/\x1B\[([0-9]{1,2}(;[0-9]{1,2})?)?[mGK]//g"
}

_casual_admin() {
	local current previous options
	COMPREPLY=()
	current="${COMP_WORDS[COMP_CWORD]}"
	previous="${COMP_WORDS[COMP_CWORD-1]}"
	options=($(casual-admin help 2>&1 | awk '/<value>/ {print $1}' | __no_colors))

	if (( COMP_CWORD == 1 )); then
		COMPREPLY=($(compgen -W "${options[*]} help" -- "${current}"))
		return 0
	elif (( COMP_CWORD == 2 )) && [[ "${options[*]}" == *"${previous}"* ]]; then
		local arguments
		arguments=($(casual-admin "${previous}" --help 2>&1 | awk '$1 ~ /[a-z],$/ {print $2}' | __no_colors))
		COMPREPLY=($(compgen -W "${arguments[*]} --help" -- "${current}"))
		return 0
	fi
}
complete -F _casual_admin casual-admin
