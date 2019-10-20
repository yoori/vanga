project(VangaDTreeTrainer)

# projects executable name
set(TARGET_NAME DTreeTrainer)

add_executable(DTreeTrainer
  Application.cpp
)

target_link_libraries(DTreeTrainer
  VangaDTree
  GearsBasic)

install(TARGETS DTreeTrainer DESTINATION bin)
