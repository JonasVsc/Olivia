#include "olivia/olivia_systems.h"
#include "olivia/olivia.h"

void input_system()
{
	context_t& ctx = *g_context;

	if (ctx.event.type == SDL_EVENT_KEY_DOWN)
		ctx.current_keys[ctx.event.key.scancode] = true;
	if (ctx.event.type == SDL_EVENT_KEY_UP)
		ctx.current_keys[ctx.event.key.scancode] = false;
}

void camera_system()
{
	context_t& ctx = *g_context;
		
	float speed = 1.0f * ctx.delta_time;

	if (is_key_down(SDL_SCANCODE_W)) ctx.camera_position.y -= speed;
	if (is_key_down(SDL_SCANCODE_S)) ctx.camera_position.y += speed;
	if (is_key_down(SDL_SCANCODE_A)) ctx.camera_position.x -= speed;
	if (is_key_down(SDL_SCANCODE_D)) ctx.camera_position.x += speed;
	if (is_key_down(SDL_SCANCODE_KP_PLUS))  ctx.camera_zoom > 0.01 ? ctx.camera_zoom -= 0.001 : 0;
	if (is_key_down(SDL_SCANCODE_KP_MINUS)) ctx.camera_zoom += 0.001;

	mat4_translate(ctx.camera_data[ctx.current_frame].view, ctx.camera_position);

	ctx.camera_data[ctx.current_frame].projection = ortho_projection(
		ctx.camera_left * ctx.camera_zoom, ctx.camera_right * ctx.camera_zoom,    // left, right
		ctx.camera_bottom * ctx.camera_zoom, ctx.camera_top * ctx.camera_zoom,    // bottom, top  
		-1.0f, 100.0f);   // near, far


	memcpy((uint8_t*)ctx.uniform_buffer[ctx.current_frame].info.pMappedData, &ctx.camera_data[ctx.current_frame], sizeof(uniform_data_t));
}

void particle_system()
{
	context_t& ctx = *g_context;

	instance3d_t& player = ctx.instances[0];

	float speed = 10.0f * ctx.delta_time;

	if (is_key_down(SDL_SCANCODE_UP))    player.transform.data[13] -= speed;
	if (is_key_down(SDL_SCANCODE_DOWN))  player.transform.data[13] += speed;
	if (is_key_down(SDL_SCANCODE_LEFT))  player.transform.data[12] -= speed;
	if (is_key_down(SDL_SCANCODE_RIGHT)) player.transform.data[12] += speed;
}


void instance_system()
{
	context_t& ctx = *g_context;

	memcpy(ctx.instances_buffer[ctx.current_frame].info.pMappedData, ctx.instances, sizeof(instance3d_t) * MAX_INSTANCES);

}