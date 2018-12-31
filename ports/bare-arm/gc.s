
	.set noat
        .set    noreorder
        .set    mips32r2
	.set nomips16
        .type   gc_savereg, @function
gc_savereg: .globl gc_savereg
	sw      $1, (0*4) ($a0)
        sw      $2, (1*4) ($a0)
        sw      $3, (2*4) ($a0)
        sw      $4, (3*4) ($a0)
        sw      $5, (4*4) ($a0)
        sw      $6, (5*4) ($a0)
        sw      $7, (6*4) ($a0)
        sw      $8, (7*4) ($a0)
        sw      $9, (8*4) ($a0)
        sw      $10, (9*4) ($a0)
        sw      $11, (10*4) ($a0)
        sw      $12, (11*4) ($a0)
        sw      $13, (12*4) ($a0)
        sw      $14, (13*4) ($a0)
        sw      $15, (14*4) ($a0)
        sw      $16, (15*4) ($a0)
        sw      $17, (16*4) ($a0)
        sw      $18, (17*4) ($a0)
        sw      $19, (18*4) ($a0)
        sw      $20, (19*4) ($a0)
        sw      $21, (20*4) ($a0)
        sw      $22, (21*4) ($a0)
        sw      $23, (22*4) ($a0)
        sw      $24, (23*4) ($a0)
        sw      $25, (24*4) ($a0)
        # Skip $26 - K0
        # Skip $27 - K1
        sw      $28, (25*4) ($a0)
        # Skip $29 - SP
        sw      $30, (26*4) ($a0)

	move	$v0,	$sp
	jr	$ra
