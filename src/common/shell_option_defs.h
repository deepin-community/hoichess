/* long names... */
SHELL_DEFINE_OPTION(search_update_interval_csecs, 500);
SHELL_DEFINE_OPTION(search_timecheck_interval_csecs_min, 2);
SHELL_DEFINE_OPTION(search_timecheck_interval_csecs_max, 5);

SHELL_DEFINE_OPTION(search_extend_time_iteration_enable, 1);
SHELL_DEFINE_OPTION(search_extend_time_iteration_min_percent_done, 75);
SHELL_DEFINE_OPTION(search_extend_time_iteration_expect_safety_factor_tenth, 11);
SHELL_DEFINE_OPTION(search_extend_time_iteration_extend_safety_factor_tenth, 10);
SHELL_DEFINE_OPTION(search_time_for_new_iteration_enable, 1);
SHELL_DEFINE_OPTION(search_time_for_new_iteration_expect_factor_tenth, 15);

SHELL_DEFINE_OPTION(search_parallel_min_depth, 6);
SHELL_DEFINE_OPTION(search_parallel_min_move_ratio, 2);
SHELL_DEFINE_OPTION(search_parallel_hash_pvtable, 0);
SHELL_DEFINE_OPTION(search_parallel_shared_hash, 0);
SHELL_DEFINE_OPTION(search_parallel_pvs_mode, 1);

SHELL_DEFINE_OPTION(search_failsoft, 0);
SHELL_DEFINE_OPTION(search_pvs_mode, 1);
