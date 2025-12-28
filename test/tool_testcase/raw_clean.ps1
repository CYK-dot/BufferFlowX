echo "test 1 ------------------------------------------------------------"
python ..\..\bfx\section\gcc\bfx_section_main.py --project-root .\section\multi_component --ld-template  .\section\multi_component\stm32cubemx.ld --output-dir .\section\multi_component\output --output-ld .\section\multi_component\output\generated.ld --clean
echo "test 2 ------------------------------------------------------------"
python ..\..\bfx\section\gcc\bfx_section_main.py --project-root .\section\conflict_in_same_dir --ld-template  .\section\conflict_in_same_dir\stm32cubemx.ld --output-dir .\section\conflict_in_same_dir\output --output-ld .\section\conflict_in_same_dir\output\generated.ld --clean
echo "test 3 ------------------------------------------------------------"
python ..\..\bfx\section\gcc\bfx_section_main.py --project-root .\section\conflict_in_diff_dir --ld-template  .\section\conflict_in_diff_dir\stm32cubemx.ld --output-dir .\section\conflict_in_diff_dir\output --output-ld .\section\conflict_in_diff_dir\output\generated.ld --clean
