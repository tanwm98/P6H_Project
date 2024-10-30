add_executable(pwm_adc
pwm_adc.c
)

# Pull in common dependencies and additional PWM hardware support
target_link_libraries(pwm_adc pico_stdlib hardware_pwm hardware_gpio hardware_timer hardware_adc)

# Enable USB output for printf
pico_enable_stdio_usb(pwm_adc 1)

# Create map/bin/hex/uf2 files for flashing
pico_add_extra_outputs(pwm_adc)

# Add URL via pico_set_program_url (optional)
example_auto_set_url(pwm_adc)
