#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "ganglia.h"
#include "cmdline.h"

Ganglia_pool global_context;
Ganglia_gmetric gmetric;
Ganglia_gmond_config gmond_config;
Ganglia_udp_send_channels send_channels;

/* The commandline options */
struct gengetopt_args_info args_info;

int
main( int argc, char *argv[] )
{
  int rval;

  /* process the commandline options */
  if (cmdline_parser (argc, argv, &args_info) != 0)
        exit(1);

  /* create the global context */
  global_context = Ganglia_pool_create(NULL);
  if(!global_context)
    {
      fprintf(stderr,"Unable to create global context. Exiting.\n");
      exit(1);
    }
  
  /* parse the configuration file */
  gmond_config = Ganglia_gmond_config_create( args_info.conf_arg, !args_info.conf_given);


  /* build the udp send channels */
  send_channels = Ganglia_udp_send_channels_create(global_context, gmond_config);
  if(!send_channels)
    {
      fprintf(stderr,"Unable to create ganglia send channels. Exiting.\n");
      exit(1);
    }

  /* create the message */
  gmetric = Ganglia_gmetric_create(global_context);
  if(!gmetric)
    {
      fprintf(stderr,"Unable to allocate gmetric structure. Exiting.\n");
      exit(1);
    }
  // Yemi 
  if(args_info.spoof_given && args_info.heartbeat_given){
      rval = 0;
  }else{
      if( ! (args_info.name_given && args_info.value_given && 
	     args_info.type_given)){
          fprintf(stderr,"Incorrect options supplied, exiting.\n");
          exit(1);
      }
      rval = Ganglia_gmetric_set( gmetric, args_info.name_arg, args_info.value_arg,
			      args_info.type_arg, args_info.units_arg, !strcmp(args_info.slope_arg,"zero")? 0: 3,
			      args_info.tmax_arg, args_info.dmax_arg);
  }
  /* TODO: make this less ugly later */
  switch(rval)
    {
    case 1:
      fprintf(stderr,"gmetric parameters invalid. exiting.\n");
      exit(1); 
    case 2:
      fprintf(stderr,"one of your parameters has an invalid character '\"'. exiting.\n");
      exit(1);
    case 3:
      fprintf(stderr,"the type parameter \"%s\" is not a valid type. exiting.\n", args_info.type_arg);
      exit(1);
    case 4:
      fprintf(stderr,"the value parameter \"%s\" does not represent a number. exiting.\n", args_info.value_arg);
      exit(1);
    }

  /* send the message */
  //Yemi
  if(!strlen(args_info.spoof_arg))
    {
      rval = Ganglia_gmetric_send(gmetric, send_channels);
    }else{
      rval = Ganglia_gmetric_send_spoof(gmetric, send_channels,args_info.spoof_arg,args_info.heartbeat_given);
    }
  if(rval)
    {
      fprintf(stderr,"There was an error sending to %d of the send channels.\n", rval);
    }

  /* cleanup */
  Ganglia_gmetric_destroy(gmetric); /* not really necessary but for symmetry */
  Ganglia_pool_destroy(global_context);

  return 0;
}