struct env_work
{
	char *saved_data[2];
	unsigned saved_len[2];
	ulong fl_sect_start;
	ulong fl_env_start;
	ulong fl_env_end;
	ulong fl_sect_end;
};
#define ACTIVE_FLAG   1

static int inline template_saveenv(unsigned env_size, ulong env_start1, ulong env_start2, env_t *envp)
{
	int rc = 1;
	int rc1;
	int i, j;
	struct env_work work[2];
	const int num_env_areas = (env_start2) ? 2 : 1;
	work[0].fl_env_start = env_start1;
	work[1].fl_env_start = env_start2;
	for (i = 0; i < num_env_areas; i++) {
		work[i].saved_data[0] = work[i].saved_data[1] = NULL;
	}
	for (i = 0; i < num_env_areas; i++) {
		work[i].fl_env_end = work[i].fl_env_start + env_size - 1;
		work[i].fl_sect_start = work[i].fl_env_start;
		work[i].fl_sect_end = work[i].fl_env_end;
		rc = TEMPLATE_sect_bounds(&work[i].fl_sect_start, &work[i].fl_sect_end); 
		if (rc) {
			printf("sect_bounds failed with %d\n", rc);
			goto Done;
		}
		printf("Environment section is %lx(%lx-%lx)%lx\n", work[i].fl_sect_start, work[i].fl_env_start, work[i].fl_env_end, work[i].fl_sect_end);
	}
	for (i = 0; i < num_env_areas; i++) {
		work[i].saved_len[0] = work[i].fl_env_start - work[i].fl_sect_start;
		work[i].saved_len[1] = work[i].fl_sect_end - work[i].fl_env_end;
		for (j = 0; j < 2; j++) {
			if (work[i].saved_len[j]) {
				ulong data_start;
				if ((work[i].saved_data[j] = malloc(work[i].saved_len[j])) == NULL) {
					printf("Unable to save the rest of sector (%d)\n",
							work[i].saved_len[j]);
					goto Done;
				}
				data_start = (j==0) ? work[i].fl_sect_start : work[i].fl_env_end + 1;
				TEMPLATE_read(work[i].saved_data[j], data_start,
					work[i].saved_len[j]);
				debug("Data (start 0x%lx, len 0x%x) saved at %p\n",
						data_start, work[i].saved_len[j], work[i].saved_data[j]);
			}
		}
	}

	for (i = 0; i < num_env_areas; i++) {
		debug("Protect off %08lX ... %08lX\n", work[i].fl_sect_start, work[i].fl_sect_end);
		if (TEMPLATE_sect_protect(0, work[i].fl_sect_start, work[i].fl_sect_end))
			goto Done;

		puts("Erasing " TEMPLATE_NAME "...");
		debug(" %08lX ... %08lX ...",
				work[i].fl_sect_start, work[i].fl_sect_end);

		if (TEMPLATE_sect_erase(work[i].fl_sect_start, work[i].fl_sect_end))
			goto Done;


		puts("Writing to " TEMPLATE_NAME "... ");
#ifdef CONFIG_SYS_REDUNDAND_ENVIRONMENT
		envp->flags = ~0;
#endif
		rc = TEMPLATE_write((char *)envp, work[i].fl_env_start, sizeof(*envp));
#ifdef CONFIG_SYS_REDUNDAND_ENVIRONMENT
		envp->flags = ACTIVE_FLAG;
		rc1 = TEMPLATE_write(&envp->flags, work[i].fl_env_start + offsetof(env_t, flags),
			    sizeof(envp->flags));
		if (!rc)
			rc = rc1;
#endif
		for (j = 0; j < 2; j++) {
			if (work[i].saved_len[j]) {
				ulong data_start;
				data_start = (j==0) ? work[i].fl_sect_start : work[i].fl_env_end + 1;
				debug("Restoring data to 0x%lx len 0x%x from %p\n",
					data_start, work[i].saved_len[j], work[i].saved_data[j]);
				rc1 = TEMPLATE_write(work[i].saved_data[j],
					data_start, work[i].saved_len[j]);
				if (!rc)
					rc = rc1;
			}
		}
		if (rc) {
			TEMPLATE_perror(rc);
			goto Done;
		}
	}
	puts("done\n");
Done:
	for (i = 0; i < num_env_areas; i++) {
		for (j = 0; j < 2; j++) {
			if (work[i].saved_data[j])
				free(work[i].saved_data[j]);
		}
	}
	/* try to re-protect */
	for (i = 0; i < num_env_areas; i++) {
		TEMPLATE_sect_protect(1, work[i].fl_sect_start, work[i].fl_sect_end);
	}
	return rc;
}
