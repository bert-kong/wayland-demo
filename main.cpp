#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<xf86drm.h>
#include<xf86drmMode.h>
#include<iostream>

#include "swapchain.hxx"


#if 1
void dump_crtc(const drmModeCrtc *crtc)
{
	printf("debug bckong ---> valid ? = %d\n", crtc->mode_valid);
	printf("debug bckong ---> CRTC object id = 0x%08X\n", crtc->crtc_id);
	printf("debug bckong ---> buffer object id = 0x%08X\n", crtc->buffer_id);
	printf("debug bckong ---> rectangle(%d, %d, %d, %d)\n", crtc->x, crtc->y, crtc->width, crtc->height);
}

void traverse_drm(const int fd)
{
	/* get number of planes and plane IDs */
	drmModePlaneRes *plane_res = drmModeGetPlaneResources(fd);

	if (plane_res==nullptr)
	{
		std::runtime_error("DRM mode get plane resources failed\n");
	}


	printf("=================== Planes =================\n");
	for (int i=0; i<plane_res->count_planes; i++)
	{
		/* get the plane object using plane ID */
		drmModePlane *plane = drmModeGetPlane(fd, plane_res->planes[i]);

		printf("debug ---> (plane, crtc) = (0x%08X::0x%08X\n", plane->plane_id, plane->crtc_id);
		printf("	 formats = %d src(%d, %d),  crtc(%d, %d)\n",
					   plane->count_formats, plane->x, plane->y, plane->crtc_x, plane->crtc_y);

		drmModeFreePlane(plane);
	}

	drmModeFreePlaneResources(plane_res);

	drmModeRes *res = drmModeGetResources(fd);
	if (res==nullptr)
	{
		throw std::runtime_error("drm mode get resources failed");
	}

	printf("=================== CRTCs =================\n");
	drmModeCrtc *crtc;
	for (int i=0; i<res->count_crtcs; i++)
	{
		crtc = drmModeGetCrtc(fd, res->crtcs[i]);
		if (crtc)
		{
			printf("debug ---> ID 0x%08X:%d (%d, %d, %d, %d)\n",
					crtc->crtc_id, crtc->mode_valid, crtc->x, crtc->y, crtc->width, crtc->height);

			if (crtc->mode.name)
			{
				printf("	name = %s\n", crtc->mode.name);
			}
			else
			{
				printf("	name = N/A\n");
			}
		}
		else
		{
			printf("debug ---> ID 0x%08X  valid = %d\n", crtc->crtc_id, crtc->mode_valid);
		}

		drmModeFreeCrtc(crtc);
	}

	printf("\n=================== Encoders =================\n");

	for (int i=0; i<res->count_encoders; i++)
	{
		drmModeEncoder *encoder = drmModeGetEncoder(fd, res->encoders[i]);

		if (encoder)
		{
			printf("debug ---> (id, type), crtc id - (0x%08X, 0x%08X), 0x%08X\n",
					encoder->encoder_id, encoder->encoder_type, encoder->crtc_id);
		}
		else
		{
			printf("debug ---> no encoder object 0x%08X\n", res->encoders[i]);
		}
	}

	printf("\n=================== Connector =================\n");
	for (int i=0; i<res->count_connectors; i++)
	{
		drmModeConnector *connector = drmModeGetConnector(fd, res->connectors[i]);
		if (connector==nullptr)
		{
			printf("debug ---> NULL connector\n");
			continue;
		}

		switch(connector->connector_type)
		{
		case DRM_MODE_CONNECTOR_DSI:
			printf("debug ---> connector type DSI 0x%08X\n", connector->connector_type);
			break;
		case DRM_MODE_CONNECTOR_HDMIA:
			printf("debug ---> connector type HDMI A 0x%08X\n", connector->connector_type);
			break;
		case DRM_MODE_CONNECTOR_HDMIB:
			printf("debug ---> connector type HDMI B 0x%08X\n", connector->connector_type);
			break;
		case DRM_MODE_CONNECTOR_DisplayPort:
			printf("debug ---> connector type DP 0x%08X\n", connector->connector_type);
			break;
		case DRM_MODE_CONNECTOR_VIRTUAL:
			printf("debug ---> connector type virtual 0x%08X\n", connector->connector_type);
			break;
		default:
			printf("debug ---> connector type Unknown 0x%08X\n", connector->connector_type);
		}

		if (connector->connection==DRM_MODE_CONNECTED)
		{
			printf("------- Connector is connected --------\n");

			for (int i=0; i<connector->count_encoders; i++)
			{
				printf("	encoder ID = 0x%08X\n",  connector->encoders[i]);
			}

			for (int i=0; i<connector->count_modes; i++)
			{
				printf(" 	name %s,	width, height %d, %d\n",
						connector->modes[i].name, connector->modes[i].hdisplay, connector->modes[i].vdisplay);
			}
			printf("------- Connector properties --------\n");
			for (int i=0; i<connector->count_props; i++)
			{
				drmModePropertyPtr prop=drmModeGetProperty(fd, connector->props[i]);
				if (prop)
				{
					printf("       	property = %s\n", prop->name);
				}
				else
				{
					printf(" 		prop N/A\n");
				}

				drmModeFreeProperty(prop);
			}
		}
		else
		{
			printf("------- Connector is NOT connected --------\n");
		}
		printf("\n\n");

		drmModeFreeConnector(connector);
	}

	drmModeFreeResources(res);
}

int test_drm()
{
	struct drm_mode_create_dumb creq;
	struct drm_mode_destroy_dumb dreq;
	struct drm_mode_map_dumb mreq;

	uint32_t fb;
	int ret;
	void *map;
    int fd, err;
    const char fname[] = "/dev/dri/card0";
    drmModePlaneRes *res;
    drmModePlane *plane;
    drmModeCrtc *crtc;

	memset(&creq, 0, sizeof(creq));

    fd = open(fname, O_RDWR);
    if (fd<0)
    {
        fprintf(stderr, "debug bckong --->  open %s failed\n", fname);
        return 1;
    }

    traverse_drm(fd);

#if 0
    /**
     * get plane resources : number of planes and plane IDs
     */
    res = drmModeGetPlaneResources(fd);

    if (res==NULL)
    {
        fprintf(stderr, "debug bckong ---> drm client cap univeral plane\n");
        return 1;
    }

    printf("debug bckong ---> drmModeGetPlaneResources success\n");

    /**
     * loop through plane ID and get corresponding plane object
     * plane attributes:
     * 	  - scaling,
     * 	  - sharpening,
     * 	  - CSC
     * 	  - src/dst rectangle
     * 	  - address, stride, formats
     */
    for (int i = 0; i<res->count_planes; i++)
    {
    	printf("debug bckong ---> plane id 0x%08X\n", res->planes[i]);

    	/* get plane object using the plane ID */
    	plane = drmModeGetPlane(fd, res->planes[i]);
    	if (plane==NULL)
    	{
    		fprintf(stderr, "debug bckong ---> failed to get plane obj through plane ID 0x%08X\n",
    				res->planes[i]);
    		std::runtime_error("failed to get plane object using the ID");
    	}
    	crtc = drmModeGetCrtc(fd, plane->crtc_id);

    	if (crtc)
    	{
    		dump_crtc(crtc);
    	}
    	else
    	{
    		fprintf(stderr, "plane has no an associated CRTC 0x%08X\n", plane->crtc_id);
    	}
    }
    drmModeFreeCrtc(crtc);

    drmModeFreePlane(plane);
    drmModeFreePlaneResources(res);
#endif

    close(fd);
    return 0;
}
#endif


int main(int argc, char *argv[]) {
#if 0
    return test_drm();
#endif


    Swapchain swapchain;

    if (argc>1) {
        switch(argv[1][0]) {
            case 'b':
               swapchain.setMode(Swapchain::DRAW_BALL);
               break;
            case 'c':
               swapchain.setMode(Swapchain::DRAW_COLOR);
               break;
            case 't':
               swapchain.setMode(Swapchain::DRAW_FRACTAL);
               break;
            default:
               swapchain.setMode(Swapchain::DRAW_BALL);
        }
    }

    swapchain.createSurface();
    swapchain.createBuffer();
    
    while (true) {

        /* send an update event to server */
        if (wl_display_dispatch_pending(swapchain.display())<0) {
            printf("Wayland display dispatch failed\n");
            break;
        }

        const int index = swapchain.swapchainAcquireBuffer();
        swapchain.draw(index);
        swapchain.swapchainPresentBuffer(index);
    }

    return 0;
}
